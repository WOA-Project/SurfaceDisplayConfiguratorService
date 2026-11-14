// BrightnessManager.cpp

#define _WIN32_DCOM

#include "pch.h"
#define IUnknown ::IUnknown

#include "BrightnessManager.h"

#include <SetupAPI.h>
#include <cfgmgr32.h>
#include <initguid.h>

#include <comdef.h>
#include <Devpkey.h>
#include <devpropdef.h>

#include <vector>

#include <vector>
#include <algorithm>

#pragma comment(lib, "Cfgmgr32.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "setupapi.lib")

DEFINE_GUID(
    GUID_DEVINTERFACE_MONITOR,
    0xe6f07b5f,
    0xee97,
    0x4a90,
    0xb0, 0x76, 0x33, 0xf5, 0x7b, 0xf4, 0xea, 0xa7);

BrightnessManager::BrightnessManager()
{}

BrightnessManager::~BrightnessManager()
{
    StopBrightnessMonitoring();

    if (m_services)
    {
        m_services->Release();
        m_services = nullptr;
    }

    if (m_locator)
    {
        m_locator->Release();
        m_locator = nullptr;
    }

    CoUninitialize();
}

bool BrightnessManager::Initialize()
{
    HRESULT hr =
        CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);

    if (FAILED(hr) &&
        hr != RPC_E_CHANGED_MODE)
    {
        return false;
    }

    hr =
        CoInitializeSecurity(
            nullptr,
            -1,
            nullptr,
            nullptr,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            nullptr,
            EOAC_NONE,
            nullptr);

    if (FAILED(hr) &&
        hr != RPC_E_TOO_LATE)
    {
        return false;
    }

    return ConnectWmi() &&
        BuildMonitorCache();
}

bool BrightnessManager::ConnectWmi()
{
    HRESULT hr =
        CoCreateInstance(
            CLSID_WbemLocator,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator,
            reinterpret_cast<void**>(
                &m_locator));

    if (FAILED(hr))
    {
        return false;
    }

    hr =
        m_locator->ConnectServer(
            _bstr_t(L"ROOT\\WMI"),
            nullptr,
            nullptr,
            nullptr,
            0,
            nullptr,
            nullptr,
            &m_services);

    if (FAILED(hr))
    {
        return false;
    }

    hr =
        CoSetProxyBlanket(
            m_services,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            nullptr,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            nullptr,
            EOAC_NONE);

    return SUCCEEDED(hr);
}

HRESULT BrightnessManager::EnumerateWmiClass(
    LPCWSTR query,
    IEnumWbemClassObject** enumerator)
{
    return m_services->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(query),
        WBEM_FLAG_FORWARD_ONLY,
        nullptr,
        enumerator);
}

bool BrightnessManager::BuildMonitorCache()
{
    m_monitors.clear();

    struct WmiMonitor
    {
        std::wstring Instance;
    };

    std::vector<WmiMonitor> wmiMonitors;

    IEnumWbemClassObject* enumerator = nullptr;

    HRESULT hr =
        EnumerateWmiClass(
            L"SELECT InstanceName FROM WmiMonitorID",
            &enumerator);

    if (FAILED(hr))
    {
        return false;
    }

    IWbemClassObject* obj = nullptr;
    ULONG returned = 0;

    while (enumerator->Next(
        WBEM_INFINITE,
        1,
        &obj,
        &returned) == S_OK)
    {
        VARIANT value;
        VariantInit(&value);

        obj->Get(
            L"InstanceName",
            0,
            &value,
            nullptr,
            nullptr);

        if (value.vt == VT_BSTR)
        {
            WmiMonitor m;
            m.Instance = value.bstrVal;

            wmiMonitors.push_back(
                std::move(m));
        }

        VariantClear(&value);

        obj->Release();
    }

    enumerator->Release();

    DISPLAY_DEVICEW adapter{};
    adapter.cb = sizeof(adapter);

    size_t index = 0;

    for (DWORD adapterIndex = 0;
        EnumDisplayDevicesW(
            nullptr,
            adapterIndex,
            &adapter,
            0);
        ++adapterIndex)
    {
        if (!(adapter.StateFlags &
            DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {
            continue;
        }

        if (index >=
            wmiMonitors.size())
        {
            break;
        }

        MonitorInfo info;

        info.GdiName =
            adapter.DeviceName;

        info.WmiInstance =
            wmiMonitors[index].Instance;

        m_monitors.push_back(
            std::move(info));

        ++index;
    }

    return !m_monitors.empty();
}

bool BrightnessManager::ResolveDisplayToWmiInstance(
    const std::wstring& gdiDisplay,
    std::wstring& instance)
{
    for (const auto& monitor :
        m_monitors)
    {
        if (_wcsicmp(
            monitor.GdiName.c_str(),
            gdiDisplay.c_str()) == 0)
        {
            instance =
                monitor.WmiInstance;

            return true;
        }
    }

    return false;
}

bool BrightnessManager::GetBrightness(
    const std::wstring& display,
    BYTE& brightness)
{
    std::wstring instance;

    if (!ResolveDisplayToWmiInstance(
        display,
        instance))
    {
        return false;
    }

    IEnumWbemClassObject* enumerator = nullptr;

    if (FAILED(
        EnumerateWmiClass(
            L"SELECT * FROM WmiMonitorBrightness",
            &enumerator)))
    {
        return false;
    }

    IWbemClassObject* obj = nullptr;
    ULONG returned = 0;

    while (enumerator->Next(
        WBEM_INFINITE,
        1,
        &obj,
        &returned) == S_OK)
    {
        VARIANT name;
        VariantInit(&name);

        obj->Get(
            L"InstanceName",
            0,
            &name,
            nullptr,
            nullptr);

        if (name.vt == VT_BSTR &&
            !_wcsicmp(
                name.bstrVal,
                instance.c_str()))
        {
            VARIANT value;
            VariantInit(&value);

            obj->Get(
                L"CurrentBrightness",
                0,
                &value,
                nullptr,
                nullptr);

            brightness =
                static_cast<BYTE>(
                    value.uintVal);

            VariantClear(&value);
            VariantClear(&name);

            obj->Release();
            enumerator->Release();

            return true;
        }

        VariantClear(&name);

        obj->Release();
    }

    enumerator->Release();

    return false;
}

bool BrightnessManager::SetBrightness(
    const std::wstring& display,
    BYTE brightness)
{
    brightness =
        static_cast<BYTE>(
            std::clamp<int>(
                brightness,
                0,
                100));

    std::wstring instance;

    if (!ResolveDisplayToWmiInstance(
        display,
        instance))
    {
        return false;
    }

    IEnumWbemClassObject* enumerator = nullptr;

    if (FAILED(
        EnumerateWmiClass(
            L"SELECT * FROM WmiMonitorBrightnessMethods",
            &enumerator)))
    {
        return false;
    }

    IWbemClassObject* obj = nullptr;
    ULONG returned = 0;

    while (enumerator->Next(
        WBEM_INFINITE,
        1,
        &obj,
        &returned) == S_OK)
    {
        VARIANT current;
        VariantInit(&current);

        obj->Get(
            L"InstanceName",
            0,
            &current,
            nullptr,
            nullptr);

        if (current.vt == VT_BSTR &&
            !_wcsicmp(
                current.bstrVal,
                instance.c_str()))
        {
            VARIANT path;
            VariantInit(&path);

            obj->Get(
                L"__PATH",
                0,
                &path,
                nullptr,
                nullptr);

            IWbemClassObject* cls = nullptr;
            IWbemClassObject* sig = nullptr;
            IWbemClassObject* params = nullptr;

            HRESULT hr =
                m_services->GetObject(
                    _bstr_t(
                        L"WmiMonitorBrightnessMethods"),
                    0,
                    nullptr,
                    &cls,
                    nullptr);

            if (SUCCEEDED(hr))
            {
                hr =
                    cls->GetMethod(
                        L"WmiSetBrightness",
                        0,
                        &sig,
                        nullptr);
            }

            if (SUCCEEDED(hr))
            {
                hr =
                    sig->SpawnInstance(
                        0,
                        &params);
            }

            if (SUCCEEDED(hr))
            {
                VARIANT timeout{};
                timeout.vt = VT_UI1;
                timeout.bVal = 0;

                VARIANT level{};
                level.vt = VT_UI1;
                level.bVal = brightness;

                params->Put(
                    L"Timeout",
                    0,
                    &timeout,
                    0);

                params->Put(
                    L"Brightness",
                    0,
                    &level,
                    0);

                hr =
                    m_services->ExecMethod(
                        path.bstrVal,
                        _bstr_t(
                            L"WmiSetBrightness"),
                        0,
                        nullptr,
                        params,
                        nullptr,
                        nullptr);
            }

            if (params) params->Release();
            if (sig)    sig->Release();
            if (cls)    cls->Release();

            VariantClear(&path);
            VariantClear(&current);

            obj->Release();
            enumerator->Release();

            return SUCCEEDED(hr);
        }

        VariantClear(&current);

        obj->Release();
    }

    enumerator->Release();

    return false;
}

bool BrightnessManager::StartBrightnessMonitoring(
    BrightnessChangedCallback callback)
{
    m_callback =
        std::move(callback);

    m_stopMonitoring = false;

    m_cachedBrightness.clear();

    for (const auto& monitor :
        m_monitors)
    {
        BYTE value = 0;

        if (GetBrightness(
            monitor.GdiName,
            value))
        {
            m_cachedBrightness[
                monitor.GdiName] =
                value;
        }
    }

    m_monitorThread =
        std::thread(
            [this]()
            {
                while (!m_stopMonitoring)
                {
                    BuildMonitorCache();

                    for (const auto& monitor :
                        m_monitors)
                    {
                        BYTE current = 0;

                        if (!GetBrightness(
                            monitor.GdiName,
                            current))
                        {
                            continue;
                        }

                        auto it =
                            m_cachedBrightness.find(
                                monitor.GdiName);

                        if (it ==
                            m_cachedBrightness.end())
                        {
                            m_cachedBrightness[
                                monitor.GdiName] =
                                current;

                                continue;
                        }

                        if (it->second != current)
                        {
                            BYTE oldValue =
                                it->second;

                            it->second =
                                current;

                            if (m_callback)
                            {
                                m_callback(
                                    monitor.GdiName,
                                    oldValue,
                                    current);
                            }
                        }
                    }

                    Sleep(1000);
                }
            });

    return true;
}

void BrightnessManager::StopBrightnessMonitoring()
{
    m_stopMonitoring = true;

    if (m_monitorThread.joinable())
    {
        m_monitorThread.join();
    }
}