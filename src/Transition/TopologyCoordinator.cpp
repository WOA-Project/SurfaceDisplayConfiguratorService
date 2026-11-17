#include "pch.h"

#include "Transition\TopologyCoordinator.h"

#include "Transition\TransitionStatistics.h"
#include "Transition\TransitionTrace.h"

#include <algorithm>

namespace Transition {

TopologyCoordinator::TopologyCoordinator(
    AnimationPlanner &Planner,
    TransitionAnimator &Animator,
    DesktopCaptureManager &CaptureManager) :
    m_Planner(Planner), m_Animator(Animator), m_CaptureManager(CaptureManager)
{
}

HRESULT
TopologyCoordinator::Execute(
    const Scenario &Scenario,
    std::vector<PanelVisual> &Visuals,
    const std::function<HRESULT()> &TopologyAction)
{
    ScopedTrace trace(TraceEvent::TransitionBegin, TraceEvent::TransitionEnd, L"TopologyCoordinator::Execute");

    std::vector<RegionPair> pairs;

    RETURN_IF_FAILED(m_Matcher.Match(Scenario.CurrentStates, Scenario.TargetStates, pairs));

    std::vector<AnimationPlan> plans(pairs.size());

    for (size_t i = 0; i < pairs.size(); ++i)
    {
        RETURN_IF_FAILED(m_Planner.BuildPlan(pairs[i], plans[i]));

        plans[i].DurationMs = Scenario.DurationMs;

        plans[i].CommitPoint = Scenario.CommitPoint;
    }

    const size_t visualCount = std::min(Visuals.size(), plans.size());

    for (size_t i = 0; i < visualCount; ++i)
    {
        RETURN_IF_FAILED(m_Animator.Execute(Visuals[i], plans[i]));
    }

    RETURN_IF_FAILED(m_Animator.Commit());

    AnimationClock clock;

    RETURN_IF_FAILED(clock.Start(Scenario.DurationMs, Scenario.CommitPoint));

    m_CaptureManager.Pause();

    RETURN_IF_FAILED(clock.WaitForCommitPoint());

    TransitionTrace::Write(TraceLevel::Information, TraceEvent::AnimationCommitPoint, L"Commit point reached");

    TransitionTrace::Write(TraceLevel::Information, TraceEvent::TopologyActionBegin, L"Applying topology change");

    const HRESULT topologyHr = TopologyAction();

    TransitionTrace::WriteHr(
        SUCCEEDED(topologyHr) ? TraceLevel::Information : TraceLevel::Error,
        TraceEvent::TopologyActionEnd,
        topologyHr,
        L"Topology action complete");

    RETURN_IF_FAILED(topologyHr);

    TopologyGeneration::Increment();

    TransitionStatistics::RecordTopologyChange();

    RETURN_IF_FAILED(m_CaptureManager.Rebuild());

    m_CaptureManager.Resume();

    RETURN_IF_FAILED(clock.WaitForCompletion());

    return S_OK;
}

} // namespace Transition
