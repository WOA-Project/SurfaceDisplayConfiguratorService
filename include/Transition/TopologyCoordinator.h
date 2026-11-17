#pragma once

#include "Transition\\AnimationPlanner.h"
#include "Transition\\DesktopCaptureManager.h"
#include "Transition\\DisplayRegionMatcher.h"
#include "Transition\\TransitionAnimator.h"
#include "Transition\\TransitionScenario.h"

#include <functional>
#include <vector>

namespace Transition {

struct PanelVisual;

class TopologyCoordinator
{
public:
    TopologyCoordinator(AnimationPlanner &Planner, TransitionAnimator &Animator, DesktopCaptureManager &CaptureManager);

    HRESULT Execute(
        const Scenario &Scenario,
        std::vector<PanelVisual> &Visuals,
        const std::function<HRESULT()> &TopologyAction);

private:
    AnimationPlanner &m_Planner;

    TransitionAnimator &m_Animator;

    DesktopCaptureManager &m_CaptureManager;

    DisplayRegionMatcher m_Matcher;
};

} // namespace Transition