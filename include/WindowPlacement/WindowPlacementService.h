#pragma once

#include <Windows.h>

#include "WindowMover.h"
#include "SpanManager.h"

#include "../Monitor/MonitorTopology.h"

enum class WindowPlacementPolicy
{
    FitMonitor,
    PreserveSize,
    SpanMonitors,
    MaximizeMonitor,
    MaximizeAcrossDisplays
};

class WindowPlacementService
{
public:
    WindowPlacementService(SpanManager &spanManager, MonitorTopology &topology);

    void PlaceWindow(HWND window, HMONITOR targetMonitor);

    void SetPolicy(WindowPlacementPolicy policy) { m_policy = policy; }

    WindowPlacementPolicy GetPolicy() const { return m_policy; }

private:
    SpanManager &m_spanManager;

    MonitorTopology &m_monitorTopology;

    WindowMover m_windowMover;

    WindowPlacementPolicy m_policy = WindowPlacementPolicy::MaximizeAcrossDisplays;
};