#pragma once

#include <Windows.h>

class WindowMover
{
public:
    void SizeWindowToRect(HWND window, RECT rect, bool translateToWorkArea = true);

    RECT ConstrainToMonitor(HWND window, const RECT &windowRect, const RECT &monitorRect);

    RECT ConstrainToVirtualDesktop(const RECT &windowRect);

    RECT FitOnScreen(const RECT &windowRect, const RECT &originMonitorRect, const RECT &destinationMonitorRect);
};
