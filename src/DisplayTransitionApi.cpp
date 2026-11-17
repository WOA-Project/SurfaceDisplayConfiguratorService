#include "pch.h"

#include "DisplayTransitionApi.h"

#include "Transition/TransitionManager.h"
#include "Transition/SnapshotBuilder.h"

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

    for (const auto &region : currentSnapshot.Regions)
    {
        wprintf(
            L"CURRENT: %s role=%d rot=%d rect=(%ld,%ld)-(%ld,%ld)\n",
            region.Id.c_str(),
            (int)region.Role,
            (int)region.Rotation,
            region.DesktopRect.left,
            region.DesktopRect.top,
            region.DesktopRect.right,
            region.DesktopRect.bottom);
    }

    for (const auto &region : targetSnapshot.Regions)
    {
        wprintf(
            L"TARGET: %s role=%d rot=%d rect=(%ld,%ld)-(%ld,%ld)\n",
            region.Id.c_str(),
            (int)region.Role,
            (int)region.Rotation,
            region.DesktopRect.left,
            region.DesktopRect.top,
            region.DesktopRect.right,
            region.DesktopRect.bottom);
    }

    return s_TransitionManager.ExecuteTransition(currentSnapshot, targetSnapshot, TopologyAction);
}