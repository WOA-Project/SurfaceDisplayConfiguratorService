#pragma once

#include <Windows.h>

namespace DpiHelpers {
bool
AllMonitorsHaveSameScaling();

UINT
GetMonitorDpi(HMONITOR monitor);

bool
IsPerMonitorAware(HWND window);

void
ScreenToWorkAreaCoords(HWND window, RECT &rect);
} // namespace DpiHelpers