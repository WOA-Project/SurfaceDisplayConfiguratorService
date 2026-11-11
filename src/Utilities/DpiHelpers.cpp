#include "pch.h"
#include "Utilities/DpiHelpers.h"
#include "Monitor/MonitorHelpers.h"

#include <ShellScalingApi.h>

#include <algorithm>
#include <array>

#pragma comment(lib, "Shcore.lib")

namespace {
constexpr UINT DefaultDpi = 96;

enum AwarenessLevel
{
    UNAWARE,
    SYSTEM_AWARE,
    PER_MONITOR_AWARE,
    PER_MONITOR_AWARE_V2,
    UNAWARE_GDISCALED
};

AwarenessLevel
GetAwarenessLevel(DPI_AWARENESS_CONTEXT context)
{
    const std::array levels{
        DPI_AWARENESS_CONTEXT_UNAWARE,
        DPI_AWARENESS_CONTEXT_SYSTEM_AWARE,
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE,
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2,
        DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED};

    for (size_t i = 0; i < levels.size(); ++i)
    {
        if (AreDpiAwarenessContextsEqual(levels[i], context))
        {
            return static_cast<AwarenessLevel>(i);
        }
    }

    return UNAWARE;
}
} // namespace

bool
DpiHelpers::AllMonitorsHaveSameScaling()
{
    auto monitors = MonitorHelpers::EnumerateMonitors();

    if (monitors.size() < 2)
    {
        return true;
    }

    UINT firstDpi{};

    {
        UINT dpiY{};

        if (GetDpiForMonitor(monitors.front(), MDT_EFFECTIVE_DPI, &firstDpi, &dpiY) != S_OK)
        {
            return false;
        }
    }

    for (size_t i = 1; i < monitors.size(); ++i)
    {
        UINT dpiX{};
        UINT dpiY{};

        if (GetDpiForMonitor(monitors[i], MDT_EFFECTIVE_DPI, &dpiX, &dpiY) != S_OK)
        {
            return false;
        }

        if (dpiX != firstDpi)
        {
            return false;
        }
    }

    return true;
}

UINT
DpiHelpers::GetMonitorDpi(HMONITOR monitor)
{
    if (!MonitorHelpers::IsMonitorValid(monitor))
    {
        return DefaultDpi;
    }

    UINT dpiX{};
    UINT dpiY{};

    if (GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY) != S_OK)
    {
        return DefaultDpi;
    }

    return dpiX;
}

bool
DpiHelpers::IsPerMonitorAware(HWND window)
{
    if (!window)
    {
        return false;
    }

    auto awareness = GetAwarenessLevel(GetWindowDpiAwarenessContext(window));

    return awareness >= PER_MONITOR_AWARE;
}

void
DpiHelpers::ScreenToWorkAreaCoords(HWND window, RECT &rect)
{
    HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFOEX info{};
    info.cbSize = sizeof(info);

    if (!GetMonitorInfo(monitor, &info))
    {
        return;
    }

    int offsetX = info.rcWork.left - info.rcMonitor.left;

    int offsetY = info.rcWork.top - info.rcMonitor.top;

    RECT reference = rect;

    reference.left -= offsetX;
    reference.right -= offsetX;

    reference.top -= offsetY;
    reference.bottom -= offsetY;

    monitor = MonitorFromRect(&reference, MONITOR_DEFAULTTOPRIMARY);

    info = {};
    info.cbSize = sizeof(info);

    if (!GetMonitorInfo(monitor, &info))
    {
        return;
    }

    offsetX = info.rcWork.left - info.rcMonitor.left;

    offsetY = info.rcWork.top - info.rcMonitor.top;

    rect.left -= offsetX;
    rect.right -= offsetX;

    rect.top -= offsetY;
    rect.bottom -= offsetY;

    const bool mixedScaling = !AllMonitorsHaveSameScaling();

    if (!IsPerMonitorAware(window) && mixedScaling)
    {
        rect.left = std::max(rect.left, info.rcMonitor.left);

        rect.right = std::min(rect.right, info.rcMonitor.right - offsetX);

        rect.top = std::max(rect.top, info.rcMonitor.top);

        rect.bottom = std::min(rect.bottom, info.rcMonitor.bottom - offsetY);
    }
}