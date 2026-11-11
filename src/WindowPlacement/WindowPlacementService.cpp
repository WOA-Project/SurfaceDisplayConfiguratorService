#include "pch.h"
#include "WindowPlacement/WindowPlacementService.h"
#include "Monitor/MonitorHelpers.h"
#include "Utilities/Geometry.h"
#include "Utilities/WindowHelpers.h"

WindowPlacementService::WindowPlacementService(SpanManager &spanManager, MonitorTopology &topology) :
    m_spanManager(spanManager), m_monitorTopology(topology)
{
}

void
WindowPlacementService::PlaceWindow(HWND window, HMONITOR targetMonitor)
{
    if (!IsWindow(window))
    {
        return;
    }

    if (!targetMonitor)
    {
        targetMonitor = MonitorHelpers::GetPrimaryMonitor();
    }

    HMONITOR originMonitor = WindowHelpers::GetMonitor(window);

    if (!originMonitor)
    {
        originMonitor = targetMonitor;
    }

    MONITORINFOEX originInfo = MonitorHelpers::GetMonitorInfoEx(originMonitor);

    MONITORINFOEX targetInfo = MonitorHelpers::GetMonitorInfoEx(targetMonitor);

    WINDOWPLACEMENT placement = WindowHelpers::GetPlacement(window);

    //
    // Span-across-displays mode.
    //
    if (m_policy == WindowPlacementPolicy::MaximizeAcrossDisplays)
    {
        RECT combined = m_monitorTopology.GetCombinedWorkArea();

        if (!Geometry::IsEmpty(combined))
        {
            m_spanManager.SpanWindow(window, combined);
        }

        return;
    }

    //
    // Window already on the target monitor.
    //
    if (originMonitor == targetMonitor)
    {
        RECT result = placement.rcNormalPosition;

        switch (m_policy)
        {
        case WindowPlacementPolicy::FitMonitor:

            result = m_windowMover.ConstrainToMonitor(window, result, targetInfo.rcWork);
            break;

        case WindowPlacementPolicy::PreserveSize:

            result = m_windowMover.ConstrainToVirtualDesktop(result);
            break;

        case WindowPlacementPolicy::SpanMonitors:

            result = Geometry::GetVirtualDesktopRect();
            break;

        case WindowPlacementPolicy::MaximizeMonitor:

            ShowWindow(window, SW_MAXIMIZE);
            return;

        case WindowPlacementPolicy::MaximizeAcrossDisplays:

            return;
        }

        if (!EqualRect(&result, &placement.rcNormalPosition))
        {
            m_windowMover.SizeWindowToRect(window, result);
        }

        return;
    }

    //
    // Moving between monitors.
    //
    RECT newPosition = m_windowMover.FitOnScreen(placement.rcNormalPosition, originInfo.rcWork, targetInfo.rcWork);

    switch (m_policy)
    {
    case WindowPlacementPolicy::FitMonitor:

        newPosition = m_windowMover.ConstrainToMonitor(window, newPosition, targetInfo.rcWork);
        break;

    case WindowPlacementPolicy::PreserveSize:

        newPosition = m_windowMover.ConstrainToVirtualDesktop(newPosition);
        break;

    case WindowPlacementPolicy::SpanMonitors:

        newPosition = Geometry::GetVirtualDesktopRect();
        break;

    case WindowPlacementPolicy::MaximizeMonitor: {
        RECT restore = {
            targetInfo.rcWork.left + 100,
            targetInfo.rcWork.top + 100,

            targetInfo.rcWork.left + Geometry::RectWidth(placement.rcNormalPosition),

            targetInfo.rcWork.top + Geometry::RectHeight(placement.rcNormalPosition)};

        placement.rcNormalPosition = restore;

        SetWindowPlacement(window, &placement);

        ShowWindow(window, SW_RESTORE);

        ShowWindow(window, SW_MAXIMIZE);

        return;
    }

    case WindowPlacementPolicy::MaximizeAcrossDisplays:

        return;
    }

    m_windowMover.SizeWindowToRect(window, newPosition, false);
}