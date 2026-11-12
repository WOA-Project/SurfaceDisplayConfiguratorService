#include "pch.h"

#include <SetupAPI.h>
#include <Devpkey.h>

#include <vector>

#include "DisplayIdentityCache.h"

#include "DevicePanel.h"
#include "DisplayTopology.h"

DisplayIdentityCache g_DisplayIdentityCache;

static HRESULT
ReadPanelGroup(PCWSTR PanelId, ULONG *Group)
{
    HDEVINFO DevInfo;
    SP_DEVINFO_DATA DevData = {};

    if (!Group)
    {
        return E_INVALIDARG;
    }

    *Group = 0;

    DevInfo = SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES);

    if (DevInfo == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DevData.cbSize = sizeof(DevData);

    DWORD Index = 0;

    while (SetupDiEnumDeviceInfo(DevInfo, Index++, &DevData))
    {
        DEVPROPTYPE Type;
        DWORD Size;

        SetupDiGetDevicePropertyW(DevInfo, &DevData, &DEVPKEY_Device_PanelId, &Type, nullptr, 0, &Size, 0);

        std::vector<BYTE> Buffer(Size);

        if (!SetupDiGetDevicePropertyW(
                DevInfo, &DevData, &DEVPKEY_Device_PanelId, &Type, Buffer.data(), Size, nullptr, 0))
        {
            continue;
        }

        PCWSTR CurrentPanelId = reinterpret_cast<PCWSTR>(Buffer.data());

        if (_wcsicmp(CurrentPanelId, PanelId) != 0)
        {
            continue;
        }

        ULONG Value = 0;

        if (SetupDiGetDevicePropertyW(
                DevInfo,
                &DevData,
                &DEVPKEY_Device_PanelGroup,
                &Type,
                reinterpret_cast<PBYTE>(&Value),
                sizeof(Value),
                nullptr,
                0))
        {
            *Group = Value;
        }

        SetupDiDestroyDeviceInfoList(DevInfo);

        return ERROR_SUCCESS;
    }

    SetupDiDestroyDeviceInfoList(DevInfo);

    return ERROR_NOT_FOUND;
}

HRESULT
DisplayIdentityCache::BuildIdentity(PCWSTR PanelId, DisplayIdentity &Identity)
{
    Identity.PanelId = PanelId;

    HRESULT Status = GetDisplayDevicePLD(PanelId, &Identity.Pld);

    if (FAILED(Status))
    {
        return Status;
    }

    Status = ReadPanelGroup(PanelId, &Identity.PanelGroup);

    if (FAILED(Status))
    {
        return Status;
    }

    Identity.DisplayDevice.cb = sizeof(DISPLAY_DEVICE);

    Status = GetDisplayDeviceByPanelId(PanelId, &Identity.DisplayDevice);

    if (FAILED(Status))
    {
        return Status;
    }

    return ERROR_SUCCESS;
}

HRESULT
DisplayIdentityCache::Initialize()
{
    auto Sensor = TwoPanelHingedDevicePosture::GetDefaultAsync().get();

    auto Reading = Sensor.GetCurrentPostureAsync().get();

    std::wstring Panel1Id = Reading.Panel1Id().c_str();

    std::wstring Panel2Id = Reading.Panel2Id().c_str();

    DisplayIdentity Identity1;
    DisplayIdentity Identity2;

    HRESULT Status = BuildIdentity(Panel1Id.c_str(), Identity1);

    if (FAILED(Status))
    {
        return Status;
    }

    Status = BuildIdentity(Panel2Id.c_str(), Identity2);

    if (FAILED(Status))
    {
        return Status;
    }

    //
    // Preserve current DuoWOA behavior.
    //
    if (Identity1.Pld.CabinetNumber == 1)
    {
        Identity1.IsLeft = true;
        Identity2.IsRight = true;

        m_LeftPanel = Identity1;
        m_RightPanel = Identity2;
    }
    else
    {
        Identity2.IsLeft = true;
        Identity1.IsRight = true;

        m_LeftPanel = Identity2;
        m_RightPanel = Identity1;
    }

    m_Initialized = true;

    return ERROR_SUCCESS;
}

const DisplayIdentity *
DisplayIdentityCache::FindByPanelId(PCWSTR PanelId) const
{
    if (!_wcsicmp(m_LeftPanel.PanelId.c_str(), PanelId))
    {
        return &m_LeftPanel;
    }

    if (!_wcsicmp(m_RightPanel.PanelId.c_str(), PanelId))
    {
        return &m_RightPanel;
    }

    return nullptr;
}

const DisplayIdentity &
DisplayIdentityCache::LeftPanel() const
{
    return m_LeftPanel;
}

const DisplayIdentity &
DisplayIdentityCache::RightPanel() const
{
    return m_RightPanel;
}
