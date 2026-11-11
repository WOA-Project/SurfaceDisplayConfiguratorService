#include "pch.h"

#include "Core/SpanController.h"

#include "Configuration/Configuration.h"
#include "Utilities/WindowHelpers.h"

#include <limits>

SpanController::SpanController() : m_windowPlacementService(m_spanManager, m_monitorTopology) {}

bool
SpanController::Initialize()
{
    m_monitorTopology.Refresh();

    return m_overlay.Create(GetModuleHandle(nullptr));
}

void
SpanController::Shutdown()
{
    m_overlay.Hide();

    m_dragWindow = nullptr;
    m_dragging = false;
    m_previewVisible = false;

    m_lastCursor = {LONG_MIN, LONG_MIN};
}

void
SpanController::OnDisplayConfigurationChanged()
{
    m_monitorTopology.Refresh();

    if (m_previewVisible)
    {
        m_overlay.Hide();
        m_previewVisible = false;
    }
}

void
SpanController::OnForegroundChanged(HWND window)
{
    m_launchMonitorResolver.RecordForegroundWindow(window);
}

void
SpanController::OnWindowCreated(HWND window)
{
    if (!m_windowClassifier.IsProcessable(window))
    {
        return;
    }

    HMONITOR monitor = m_launchMonitorResolver.DetermineLaunchMonitor(window);

    m_launchMonitorResolver.ClearLaunchSignal();

    m_windowPlacementService.PlaceWindow(window, monitor);
}

void
SpanController::OnMouseClick(POINT point)
{
    HWND hwnd = WindowFromPoint(point);

    if (!hwnd)
    {
        return;
    }

    m_launchMonitorResolver.RecordMouseClick(point, hwnd);

    HWND root = WindowHelpers::GetRootWindow(hwnd);

    if (m_spanManager.IsSpanned(root))
    {
        m_spanManager.RestoreWindow(root);
    }
}

void
SpanController::OnMoveSizeStart(HWND window)
{
    if (m_spanManager.IsSpanned(window))
    {
        m_spanManager.RestoreWindow(window);
    }

    m_overlay.Hide();

    m_previewVisible = false;

    m_dragging = true;
    m_dragWindow = window;

    m_lastCursor = {LONG_MIN, LONG_MIN};
}

void
SpanController::OnMoveSizeEnd(HWND window)
{
    UNREFERENCED_PARAMETER(window);

    if (!m_dragging)
    {
        return;
    }

    m_overlay.Hide();

    m_previewVisible = false;

    if (!m_dragWindow)
    {
        m_dragWindow = nullptr;
        m_dragging = false;
        return;
    }

    if (m_monitorTopology.GetSeam().Orientation == SeamOrientation::None)
    {
        m_dragWindow = nullptr;
        m_dragging = false;
        return;
    }

    POINT cursor{};

    if (!GetCursorPos(&cursor))
    {
        m_dragWindow = nullptr;
        m_dragging = false;
        return;
    }

    LONG delta = DistanceToSeam(cursor);

    if (delta < Configuration::CommitMargin && !m_spanManager.IsSpanned(m_dragWindow))
    {
        m_spanManager.SpanWindow(m_dragWindow, m_monitorTopology.GetCombinedWorkArea());
    }

    m_dragWindow = nullptr;
    m_dragging = false;
}

bool
SpanController::ShouldShowPreview(POINT cursor) const
{
    if (m_monitorTopology.GetSeam().Orientation == SeamOrientation::None)
    {
        return false;
    }

    return DistanceToSeam(cursor) < Configuration::PreviewMargin;
}

LONG
SpanController::DistanceToSeam(POINT cursor) const
{
    const auto &seam = m_monitorTopology.GetSeam();

    if (seam.Orientation == SeamOrientation::Vertical)
    {
        return abs(cursor.x - seam.Position);
    }

    if (seam.Orientation == SeamOrientation::Horizontal)
    {
        return abs(cursor.y - seam.Position);
    }

    return LONG_MAX;
}

void
SpanController::UpdateDragPreview()
{
    if (!m_dragging || !m_dragWindow)
    {
        return;
    }

    if (!IsWindow(m_dragWindow))
    {
        m_dragWindow = nullptr;
        m_dragging = false;

        m_overlay.Hide();
        m_previewVisible = false;

        return;
    }

    POINT cursor{};

    if (!GetCursorPos(&cursor))
    {
        return;
    }

    if (cursor.x == m_lastCursor.x && cursor.y == m_lastCursor.y)
    {
        return;
    }

    m_lastCursor = cursor;

    if (ShouldShowPreview(cursor))
    {
        if (!m_previewVisible)
        {
            m_overlay.Show(m_monitorTopology.GetCombinedWorkArea(), m_dragWindow);

            m_previewVisible = true;
        }
    }
    else if (m_previewVisible)
    {
        m_overlay.Hide();

        m_previewVisible = false;
    }
}

void
SpanController::OnWindowDestroyed(HWND hwnd)
{
    m_spanManager.RemoveWindow(hwnd);
}