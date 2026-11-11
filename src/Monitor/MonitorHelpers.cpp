#include "pch.h"
#include "Monitor/MonitorHelpers.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr LONG SeamTolerance = 32;

BOOL CALLBACK
CollectMonitorCallback(HMONITOR monitor, HDC, LPRECT, LPARAM data)
{
    if (!data)
    {
        return FALSE;
    }

    auto *monitors = reinterpret_cast<std::vector<HMONITOR> *>(data);

    monitors->push_back(monitor);

    return TRUE;
}
} // namespace

std::vector<HMONITOR>
MonitorHelpers::EnumerateMonitors()
{
    std::vector<HMONITOR> monitors;

    EnumDisplayMonitors(nullptr, nullptr, CollectMonitorCallback, reinterpret_cast<LPARAM>(&monitors));

    return monitors;
}

RECT
MonitorHelpers::GetCombinedWorkArea()
{
    auto monitors = EnumerateMonitors();

    if (monitors.empty())
    {
        RECT empty{};
        SetRectEmpty(&empty);

        return empty;
    }

    RECT combined = {LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN};

    for (auto monitor : monitors)
    {
        if (!IsMonitorValid(monitor))
        {
            continue;
        }

        auto info = GetMonitorInfoEx(monitor);

        combined.left = std::min(combined.left, info.rcWork.left);

        combined.top = std::min(combined.top, info.rcWork.top);

        combined.right = std::max(combined.right, info.rcWork.right);

        combined.bottom = std::max(combined.bottom, info.rcWork.bottom);
    }

    return combined;
}

RECT
MonitorHelpers::GetVirtualDesktopRect()
{
    RECT rect{};

    rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);

    rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);

    rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    return rect;
}

HMONITOR
MonitorHelpers::GetPrimaryMonitor()
{
    const POINT origin = {0, 0};

    return MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
}

bool
MonitorHelpers::AreMonitorsAdjacent(const RECT &first, const RECT &second)
{
    return std::abs(first.right - second.left) < SeamTolerance || std::abs(second.right - first.left) < SeamTolerance;
}

bool
MonitorHelpers::AreMonitorsStacked(const RECT &first, const RECT &second)
{
    return std::abs(first.bottom - second.top) < SeamTolerance || std::abs(second.bottom - first.top) < SeamTolerance;
}

MONITORINFOEX
MonitorHelpers::GetMonitorInfoEx(HMONITOR monitor)
{
    MONITORINFOEX info{};
    info.cbSize = sizeof(info);

    if (!monitor || !GetMonitorInfo(monitor, &info))
    {
        ZeroMemory(&info, sizeof(info));

        info.cbSize = sizeof(info);
    }

    return info;
}

bool
MonitorHelpers::IsMonitorValid(HMONITOR monitor)
{
    if (!monitor)
    {
        return false;
    }

    MONITORINFO info{};
    info.cbSize = sizeof(info);

    return GetMonitorInfo(monitor, &info) != FALSE;
}