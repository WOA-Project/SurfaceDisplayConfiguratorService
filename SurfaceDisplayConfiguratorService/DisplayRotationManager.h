#pragma once

DWORD WINAPI
SetExtendedDisplayConfiguration();
DWORD WINAPI
SetCorrectDisplayConfiguration();
DWORD WINAPI
SetPanelsOrientationState(SimpleOrientation Panel1SimpleOrientation, SimpleOrientation Panel2SimpleOrientation);
DWORD WINAPI
SetHardwareEnabledStateForPanel(CONST WCHAR *devicePanelId, CONST WCHAR *deviceHardwareId, BOOLEAN enabledState);