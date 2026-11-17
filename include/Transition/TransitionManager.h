#pragma once

#include "Transition\\AnimationPlanner.h"
#include "Transition\\CapturedDesktop.h"
#include "Transition\\CompositionHost.h"
#include "Transition\\DesktopCaptureManager.h"
#include "Transition\\RenderGeometry.h"
#include "Transition\\RenderGeometryBuilder.h"
#include "Transition\TransitionScenario.h"
#include "Transition\\SnapshotBuilder.h"
#include "Transition\\SurfaceFactory.h"
#include "Transition\\TopologySnapshot.h"
#include "Transition\\TransitionOverlayWindow.h"
#include "Transition\\TransitionScenarioBuilder.h"

#include <functional>
#include <vector>

namespace Transition {

struct PanelVisual;

struct Request
{
};

class TransitionManager
{
public:
    TransitionManager();

    ~TransitionManager();

    HRESULT BeginTransition(const Request &Request);

    HRESULT CommitTopology(const std::function<HRESULT()> &Callback);

    HRESULT EndTransition();

    HRESULT ExecuteTransition(
        const TopologySnapshot &CurrentSnapshot,
        const TopologySnapshot &TargetSnapshot,
        const std::function<HRESULT()> &TopologyAction);

private:
    bool m_Active = false;

    Request m_Request{};

    TransitionOverlayWindow m_Overlay;

    CompositionHost m_CompositionHost;

    DesktopCaptureManager m_CaptureManager;

    SurfaceFactory m_SurfaceFactory;

    TransitionScenarioBuilder m_ScenarioBuilder;

    AnimationPlanner m_Planner;

    RenderGeometryBuilder m_RenderGeometryBuilder;

    CapturedDesktop m_CapturedDesktop;

    std::vector<PanelVisual> m_PanelVisuals;

    std::vector<RenderGeometry> m_CurrentRenderGeometry;

    std::vector<RenderGeometry> m_TargetRenderGeometry;
};

} // namespace Transition