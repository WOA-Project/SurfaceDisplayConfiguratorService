#pragma once

#include <functional>

#include <windows.h>

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
    const std::function<HRESULT()> &TopologyAction);