#pragma once

#include "pch.h"

extern HPOWERNOTIFY g_SystemSuspendHandle;

VOID WINAPI
InitializePowerManagement(TwoPanelHingedDevicePosture PostureSensor, FlipSensor FlipSensorInstance);

VOID WINAPI
ShutdownPowerManagement();

VOID WINAPI
HandleDisplayPowerEvent(GUID SettingGuid, PVOID Value, ULONG ValueLength);

ULONG CALLBACK
SuspendResumeCallback(PVOID Context, ULONG PowerEvent, PVOID Setting);