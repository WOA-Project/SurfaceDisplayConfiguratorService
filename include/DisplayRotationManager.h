#pragma once

HRESULT WINAPI
SetHardwareEnabledStateForPanel(CONST WCHAR *devicePanelId, CONST WCHAR *deviceHardwareId, BOOLEAN enabledState);

HRESULT WINAPI
SetExtendedDisplayConfiguration();

HRESULT WINAPI
SetDisplayStates(
    CONST WCHAR *DisplayPanelId1,
    CONST WCHAR *DisplayPanelId2,
    INT DisplayOrientation1,
    INT DisplayOrientation2,
    BOOLEAN DisplayState1,
    BOOLEAN DisplayState2);

HRESULT WINAPI
SetDisplayStatesInternal(
    CONST WCHAR *LeftDisplayPanelId,
    CONST WCHAR *RightDisplayPanelId,
    INT LeftDisplayOrientation,
    INT RightDisplayOrientation,
    BOOLEAN LeftDisplayState,
    BOOLEAN RightDisplayState);