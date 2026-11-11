#pragma once

#include <Windows.h>

#include "../Monitor/MonitorTopology.h"

#include "../WindowPlacement/SpanManager.h"
#include "../WindowPlacement/WindowMover.h"
#include "../WindowPlacement/WindowPlacementService.h"

#include "../Overlay/OverlayWindow.h"

#include "../Detection/WindowClassifier.h"
#include "../Detection/LaunchMonitorResolver.h"

class SpanController
{
public:
    SpanController();

    bool Initialize();

    void Shutdown();

    void OnWindowCreated(HWND window);

    void OnMoveSizeStart(HWND window);

    void OnMoveSizeEnd(HWND window);

    void OnForegroundChanged(HWND window);

    void OnDisplayConfigurationChanged();

    void UpdateDragPreview();

    void OnWindowDestroyed(HWND hwnd);

    void OnMouseClick(POINT point);

private:
    bool ShouldShowPreview(POINT cursor) const;

    LONG DistanceToSeam(POINT cursor) const;

private:
    SpanManager m_spanManager;

    MonitorTopology m_monitorTopology;

    WindowPlacementService m_windowPlacementService;

    OverlayWindow m_overlay;

    WindowMover m_windowMover;

    WindowClassifier m_windowClassifier;

    LaunchMonitorResolver m_launchMonitorResolver;

    POINT
    m_lastCursor{};

    HWND m_dragWindow{};

    bool m_dragging{};

    bool m_previewVisible{};
};