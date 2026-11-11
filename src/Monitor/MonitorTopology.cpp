#include "pch.h"
#include "Monitor/MonitorTopology.h"
#include "Monitor/MonitorHelpers.h"
#include "Configuration/Configuration.h"

#include <algorithm>
#include <cmath>

void
MonitorTopology::Refresh()
{
    m_monitors = MonitorHelpers::EnumerateMonitors();

    m_combinedWorkArea = MonitorHelpers::GetCombinedWorkArea();

    RebuildSeam();
}

const SeamInfo &
MonitorTopology::GetSeam() const
{
    return m_seam;
}

const RECT &
MonitorTopology::GetCombinedWorkArea() const
{
    return m_combinedWorkArea;
}

const std::vector<HMONITOR> &
MonitorTopology::GetMonitors() const
{
    return m_monitors;
}

void
MonitorTopology::RebuildSeam()
{
    m_seam = {};

    if (m_monitors.size() != 2)
    {
        return;
    }

    if (!MonitorHelpers::IsMonitorValid(m_monitors[0]) || !MonitorHelpers::IsMonitorValid(m_monitors[1]))
    {
        return;
    }

    auto first = MonitorHelpers::GetMonitorInfoEx(m_monitors[0]);

    auto second = MonitorHelpers::GetMonitorInfoEx(m_monitors[1]);

    RECT a = first.rcMonitor;

    RECT b = second.rcMonitor;

    if (a.left > b.left)
    {
        std::swap(a, b);
    }

    //
    // Side-by-side displays.
    //
    if (MonitorHelpers::AreMonitorsAdjacent(a, b))
    {
        m_seam.Orientation = SeamOrientation::Vertical;

        m_seam.Position = std::abs(a.right - b.left) < std::abs(b.right - a.left) ? a.right : b.right;

        m_seam.ActivationZone = {
            m_seam.Position - Configuration::PreviewMargin,

            std::min(a.top, b.top),

            m_seam.Position + Configuration::PreviewMargin,

            std::max(a.bottom, b.bottom)};

        return;
    }

    //
    // Stacked displays.
    //
    if (MonitorHelpers::AreMonitorsStacked(a, b))
    {
        m_seam.Orientation = SeamOrientation::Horizontal;

        m_seam.Position = std::abs(a.bottom - b.top) < std::abs(b.bottom - a.top) ? a.bottom : b.bottom;

        m_seam.ActivationZone = {
            std::min(a.left, b.left),

            m_seam.Position - Configuration::PreviewMargin,

            std::max(a.right, b.right),

            m_seam.Position + Configuration::PreviewMargin};

        return;
    }
}