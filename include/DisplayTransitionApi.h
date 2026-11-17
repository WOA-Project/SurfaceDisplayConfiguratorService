#pragma once

#include <functional>

struct _devicemodeW;
typedef struct _devicemodeW DEVMODEW;
typedef DEVMODEW DEVMODE;

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