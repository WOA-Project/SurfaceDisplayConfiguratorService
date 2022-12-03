#pragma once

#include <Windows.h>

BOOL WINAPI EnableTabletPosture();
BOOL WINAPI EnableTabletMode();
BOOL WINAPI EnableTabletPostureTaskbar();

DWORD WINAPI SetCorrectDisplayConfiguration();
BOOL WINAPI AreDisplaysAlreadyConfigured();
BOOL WINAPI MarkDisplaysAlreadyConfigured();