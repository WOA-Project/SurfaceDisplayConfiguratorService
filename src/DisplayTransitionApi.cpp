#include "pch.h"

#include "DisplayTransitionApi.h"

#include "Transition/SnapshotBuilder.h"
#include "Transition/TransitionManager.h"

namespace {
Transition::TransitionManager s_TransitionManager;
}

HRESULT
ExecuteDisplayTopologyTransition(
    const wchar_t *LeftPanelId,
    const wchar_t *RightPanelId,
    const DEVMODE &CurrentLeft,
    const DEVMODE &CurrentRight,
    const DEVMODE &TargetLeft,
    const DEVMODE &TargetRight,
    bool CurrentLeftEnabled,
    bool CurrentRightEnabled,
    bool TargetLeftEnabled,
    bool TargetRightEnabled,
    const std::function<HRESULT()> &TopologyAction)
{
    auto currentSnapshot = Transition::SnapshotBuilder::BuildTopologySnapshot(
        LeftPanelId, RightPanelId, CurrentLeft, CurrentRight, CurrentLeftEnabled, CurrentRightEnabled);

    auto targetSnapshot = Transition::SnapshotBuilder::BuildTopologySnapshot(
        LeftPanelId, RightPanelId, TargetLeft, TargetRight, TargetLeftEnabled, TargetRightEnabled);

    return s_TransitionManager.ExecuteTransition(currentSnapshot, targetSnapshot, TopologyAction);
}