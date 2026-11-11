#include "pch.h"
#include "WindowPlacement/WindowMover.h"
#include "Utilities/Geometry.h"
#include "Utilities/DpiHelpers.h"
#include "Utilities/WindowHelpers.h"

namespace {
constexpr int CUSTOM_POSITIONING_PADDING = 16;
}

RECT
WindowMover::ConstrainToVirtualDesktop(const RECT &windowRect)
{
    RECT result = Geometry::ClampSizeToBounds(windowRect, Geometry::GetVirtualDesktopRect());

    return Geometry::ConstrainToBounds(result, Geometry::GetVirtualDesktopRect());
}

RECT
WindowMover::ConstrainToMonitor(HWND hwnd, const RECT &windowRect, const RECT &monitorRect)
{
    RECT result = windowRect;

    const bool canResize = WindowHelpers::HasStyle(hwnd, WS_THICKFRAME);

    if (canResize)
    {
        result = Geometry::ClampSizeToBounds(result, monitorRect);
    }

    return Geometry::ConstrainToBounds(result, monitorRect);
}

RECT
WindowMover::FitOnScreen(const RECT &windowRect, const RECT &originMonitorRect, const RECT &destinationMonitorRect)
{
    const int width = Geometry::RectWidth(windowRect);

    const int height = Geometry::RectHeight(windowRect);

    int left = destinationMonitorRect.left + (windowRect.left - originMonitorRect.left);

    int top = destinationMonitorRect.top + (windowRect.top - originMonitorRect.top);

    if (left < destinationMonitorRect.left)
    {
        left = destinationMonitorRect.left + CUSTOM_POSITIONING_PADDING;
    }

    if (left + width > destinationMonitorRect.right)
    {
        left = destinationMonitorRect.right - width - CUSTOM_POSITIONING_PADDING;
    }

    if (top < destinationMonitorRect.top)
    {
        top = destinationMonitorRect.top + CUSTOM_POSITIONING_PADDING;
    }

    if (top + height > destinationMonitorRect.bottom)
    {
        top = destinationMonitorRect.bottom - height - CUSTOM_POSITIONING_PADDING;
    }

    return {left, top, left + width, top + height};
}

void
WindowMover::SizeWindowToRect(HWND window, RECT rect, bool translateToWorkArea)
{
    if (!IsWindow(window))
    {
        return;
    }

    WINDOWPLACEMENT placement = WindowHelpers::GetPlacement(window);

    const bool wasMinimized = placement.showCmd == SW_SHOWMINIMIZED;

    const bool wasMaximized = WindowHelpers::IsMaximized(window) || placement.showCmd == SW_SHOWMAXIMIZED;

    if (wasMinimized)
    {
        ShowWindow(window, SW_RESTORE);

        placement = WindowHelpers::GetPlacement(window);
    }

    if (translateToWorkArea)
    {
        DpiHelpers::ScreenToWorkAreaCoords(window, rect);
    }

    placement.rcNormalPosition = rect;

    placement.flags |= WPF_ASYNCWINDOWPLACEMENT;

    if (!WindowHelpers::IsVisible(window))
    {
        placement.showCmd = SW_HIDE;
    }
    else if (wasMaximized)
    {
        placement.showCmd = SW_SHOWMAXIMIZED;
    }
    else
    {
        placement.showCmd = SW_RESTORE;
    }

    SetWindowPlacement(window, &placement);

    if (wasMaximized)
    {
        ShowWindow(window, SW_MAXIMIZE);
    }
}