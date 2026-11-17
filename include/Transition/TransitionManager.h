#pragma once

#include "Transition\AnimationPlanner.h"
#include "Transition\CapturedDesktop.h"
#include "Transition\CompositionHost.h"
#include "Transition\DesktopCaptureManager.h"
#include "Transition\SurfaceFactory.h"
#include "Transition\TopologyCoordinator.h"
#include "Transition\TransitionOverlayWindow.h"
#include "Transition\TransitionScenarioBuilder.h"
#include "Transition\TransitionTypes.h"

#include "InputStitching\Plan.h"

#include <functional>
#include <vector>

namespace Transition {

struct TransitionResult
{
    HRESULT Hr = S_OK;

    UINT DurationMs = 0;

    bool RebuildOccurred = false;

    bool TopologyChanged = false;
};

class TransitionManager
{
public:
    TransitionManager();

    ~TransitionManager();

    TransitionManager(const TransitionManager &) = delete;

    TransitionManager &operator=(const TransitionManager &) = delete;

    HRESULT BeginTransition(const Request &Request);

    HRESULT CommitTopology(const std::function<HRESULT()> &Callback);

    HRESULT EndTransition();

    HRESULT ExecuteTransition(
        const TopologySnapshot &CurrentSnapshot,
        const TopologySnapshot &TargetSnapshot,
        const std::function<HRESULT()> &TopologyAction);

    bool IsTransitionActive() const noexcept { return m_Active; }

private:
    bool m_Active = false;

    Request m_Request;

    TransitionOverlayWindow m_Overlay;

    CompositionHost m_CompositionHost;

    DesktopCaptureManager m_CaptureManager;

    SurfaceFactory m_SurfaceFactory;

    TransitionScenarioBuilder m_ScenarioBuilder;

    AnimationPlanner m_Planner;

    CapturedDesktop m_CapturedDesktop;

    std::vector<PanelVisual> m_PanelVisuals;
};

} // namespace Transition