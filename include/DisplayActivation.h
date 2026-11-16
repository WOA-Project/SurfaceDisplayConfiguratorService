#pragma once

#include <windows.h>

HRESULT WINAPI
DisablePanelHardwareIfNeeded(CONST WCHAR *PanelId, BOOLEAN DesiredState, BOOLEAN CurrentState);

HRESULT WINAPI
EnablePanelHardwareIfNeeded(CONST WCHAR *PanelId, BOOLEAN DesiredState, BOOLEAN CurrentState);

HRESULT WINAPI
SynchronizePanelHardwareState(
    CONST WCHAR *LeftPanelId,
    CONST WCHAR *RightPanelId,
    BOOLEAN LeftDesiredState,
    BOOLEAN RightDesiredState,
    BOOLEAN LeftCurrentState,
    BOOLEAN RightCurrentState);

HRESULT WINAPI
RestorePanelHardwareState(
    CONST WCHAR *LeftPanelId,
    CONST WCHAR *RightPanelId,
    BOOLEAN LeftDesiredState,
    BOOLEAN RightDesiredState,
    BOOLEAN LeftCurrentState,
    BOOLEAN RightCurrentState);