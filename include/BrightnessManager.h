// BrightnessManager.h

#pragma once

#define IUnknown ::IUnknown

#include <Windows.h>
#include <Wbemidl.h>

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>

class BrightnessManager
{
public:

    struct MonitorInfo
    {
        std::wstring GdiName;
        std::wstring WmiInstance;
        std::wstring DevicePath;
    };

    using BrightnessChangedCallback =
        std::function<void(
            const std::wstring& display,
            BYTE oldBrightness,
            BYTE newBrightness)>;

public:

    BrightnessManager();
    ~BrightnessManager();

    bool Initialize();

    bool GetBrightness(
        const std::wstring& display,
        BYTE& brightness);

    bool SetBrightness(
        const std::wstring& display,
        BYTE brightness);

    bool StartBrightnessMonitoring(
        BrightnessChangedCallback callback);

    void StopBrightnessMonitoring();

private:

    bool ConnectWmi();

    bool BuildMonitorCache();

    bool ResolveDisplayToWmiInstance(
        const std::wstring& gdiDisplay,
        std::wstring& instance);

    HRESULT EnumerateWmiClass(
        LPCWSTR query,
        IEnumWbemClassObject** enumerator);

private:

    IWbemLocator* m_locator = nullptr;
    IWbemServices* m_services = nullptr;

    std::vector<MonitorInfo> m_monitors;

    std::thread m_monitorThread;

    std::atomic<bool> m_stopMonitoring{ false };

    BrightnessChangedCallback m_callback;

    std::unordered_map<
        std::wstring,
        BYTE> m_cachedBrightness;
};