#include "pch.h"

#include <InputStitching\InputSpaceDisplayEnumerator.h>

#include <windows.h>
#include <ShellScalingApi.h>

#include <algorithm>
#include <atomic>
#include <vector>

#include <DevicePanel.h>
#include <DisplayIdentityCache.h>

#pragma comment(lib, "Shcore.lib")

namespace InputStitching {

namespace {

constexpr UINT32 kMaxTopologyRetries = 5;
constexpr UINT32 kMaxEnumerationRetries = 3;
constexpr UINT32 kMaxPanelIdRetries = 3;

static std::atomic_uint64_t g_EnumerationCounter{};

DWORD
RetryBackoff(UINT32 Attempt, DWORD InitialDelay, DWORD MaximumDelay)
{
    DWORD delay = InitialDelay << Attempt;

    return std::min(delay, MaximumDelay);
}

std::wstring
GetTargetName(const DISPLAYCONFIG_PATH_INFO &Path)
{
    DISPLAYCONFIG_TARGET_DEVICE_NAME name{};

    name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;

    name.header.size = sizeof(name);

    name.header.adapterId = Path.targetInfo.adapterId;

    name.header.id = Path.targetInfo.id;

    LONG status = DisplayConfigGetDeviceInfo(&name.header);

    if (status == ERROR_SUCCESS && name.monitorFriendlyDeviceName[0])
    {
        return name.monitorFriendlyDeviceName;
    }

    return L"(unnamed)";
}

HRESULT
GetSourceDeviceName(const DISPLAYCONFIG_PATH_INFO &Path, std::wstring &DeviceName)
{
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};

    source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;

    source.header.size = sizeof(source);

    source.header.adapterId = Path.sourceInfo.adapterId;

    source.header.id = Path.sourceInfo.id;

    LONG status = DisplayConfigGetDeviceInfo(&source.header);

    if (status != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(status);
    }

    DeviceName = source.viewGdiDeviceName;

    return S_OK;
}

int
GetRoleOrder(InternalPanelRole Role)
{
    switch (Role)
    {
    case InternalPanelRole::LeftPanel:

        return 0;

    case InternalPanelRole::RightPanel:

        return 1;

    case InternalPanelRole::External:

        return 2;

    default:

        return 100;
    }
}

HRESULT
EnumerateDisplaysOnce(std::vector<DisplayEntry> *Displays)
{
    std::vector<DISPLAYCONFIG_PATH_INFO> paths;

    std::vector<DISPLAYCONFIG_MODE_INFO> modes;

    LONG status = ERROR_INSUFFICIENT_BUFFER;

    for (UINT32 attempt = 0; attempt < kMaxTopologyRetries && status == ERROR_INSUFFICIENT_BUFFER; ++attempt)
    {
        UINT32 pathCount = 0;
        UINT32 modeCount = 0;

        status = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount);

        if (status != ERROR_SUCCESS)
        {
            return HRESULT_FROM_WIN32(status);
        }

        paths.resize(pathCount);
        modes.resize(modeCount);

        status = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        if (status == ERROR_SUCCESS)
        {
            paths.resize(pathCount);
            modes.resize(modeCount);
            break;
        }

        if (status == ERROR_INSUFFICIENT_BUFFER)
        {
            Sleep(RetryBackoff(attempt, 10, 250));

            continue;
        }

        return HRESULT_FROM_WIN32(status);
    }

    if (status != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(status);
    }

    for (const auto &path : paths)
    {
        if (!(path.flags & DISPLAYCONFIG_PATH_ACTIVE))
        {
            continue;
        }

        UINT32 sourceIndex = path.sourceInfo.modeInfoIdx;

        if (sourceIndex == DISPLAYCONFIG_PATH_MODE_IDX_INVALID || sourceIndex >= modes.size())
        {
            continue;
        }

        const auto &sourceMode = modes[sourceIndex];

        if (sourceMode.infoType != DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE)
        {
            continue;
        }

        const auto &source = sourceMode.sourceMode;

        DisplayEntry entry{};

        entry.Region.Rect.left = source.position.x;

        entry.Region.Rect.top = source.position.y;

        entry.Region.Rect.right = source.position.x + static_cast<LONG>(source.width);

        entry.Region.Rect.bottom = source.position.y + static_cast<LONG>(source.height);

        entry.Region.Flags = kRegionDisplay;

        if (source.position.x == 0 && source.position.y == 0)
        {
            entry.Region.Flags |= kRegionPrimary;
        }

        entry.Region.SourceAdapterId = path.sourceInfo.adapterId;

        entry.Region.SourceId = path.sourceInfo.id;

        entry.Region.TargetAdapterId = path.targetInfo.adapterId;

        entry.Region.TargetId = path.targetInfo.id;

        UINT32 rotation = static_cast<UINT32>(path.targetInfo.rotation);

        if (rotation < 1 || rotation > 4)
        {
            rotation = DISPLAYCONFIG_ROTATION_IDENTITY;
        }

        entry.Region.Rotation = rotation;

        entry.Region.ModeRect = {0, 0, static_cast<LONG>(source.width), static_cast<LONG>(source.height)};

        entry.Region.ScalingRect = entry.Region.ModeRect;

        HMONITOR monitor = MonitorFromRect(&entry.Region.Rect, MONITOR_DEFAULTTONULL);

        if (monitor)
        {
            UINT dpiX = 96;
            UINT dpiY = 96;

            if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
            {
                entry.Region.Dpi = static_cast<UINT16>(dpiX);

                entry.Region.DpiAlt = static_cast<UINT16>(dpiY);
            }
        }

        std::wstring deviceName;

        if (SUCCEEDED(GetSourceDeviceName(path, deviceName)))
        {
            auto identity = g_DisplayIdentityCache.FindByDisplayName(deviceName.c_str());

            if (identity)
            {
                entry.PanelId = identity->PanelId;

                entry.Identity = identity;

                entry.Internal = true;

                if (identity->IsLeft)
                {
                    entry.Role = InternalPanelRole::LeftPanel;
                }
                else if (identity->IsRight)
                {
                    entry.Role = InternalPanelRole::RightPanel;
                }
            }
        }

        entry.Name = GetTargetName(path);

        if (!entry.Identity)
        {
            entry.Role = InternalPanelRole::External;

            entry.Internal = false;
        }

        Displays->push_back(std::move(entry));
    }

    std::stable_sort(Displays->begin(), Displays->end(), [](const DisplayEntry &Left, const DisplayEntry &Right) {
        return GetRoleOrder(Left.Role) < GetRoleOrder(Right.Role);
    });

    bool leftFound = false;
    bool rightFound = false;

    for (const auto &display : *Displays)
    {
        if (display.Role == InternalPanelRole::LeftPanel)
        {
            leftFound = true;
        }

        if (display.Role == InternalPanelRole::RightPanel)
        {
            rightFound = true;
        }
    }

    if (Displays->empty())
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return S_OK;
}

} // namespace

HRESULT
EnumerateDisplays(std::vector<DisplayEntry> *Displays)
{
    if (!Displays)
    {
        return E_POINTER;
    }

    ++g_EnumerationCounter;

    HRESULT hr = E_FAIL;

    for (UINT32 attempt = 0; attempt < kMaxEnumerationRetries; ++attempt)
    {
        Displays->clear();

        hr = EnumerateDisplaysOnce(Displays);

        if (SUCCEEDED(hr) && !Displays->empty())
        {
            return hr;
        }

        Sleep(RetryBackoff(attempt, 25, 250));
    }

    return hr;
}

} // namespace InputStitching