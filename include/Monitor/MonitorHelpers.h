#pragma once

#include <Windows.h>

#include <vector>

namespace MonitorHelpers {
std::vector<HMONITOR>
EnumerateMonitors();

RECT
GetCombinedWorkArea();

RECT
GetVirtualDesktopRect();

HMONITOR
GetPrimaryMonitor();

bool
IsMonitorValid(HMONITOR monitor);

bool
AreMonitorsAdjacent(const RECT &first, const RECT &second);

bool
AreMonitorsStacked(const RECT &first, const RECT &second);

MONITORINFOEX
GetMonitorInfoEx(HMONITOR monitor);
} // namespace MonitorHelpers