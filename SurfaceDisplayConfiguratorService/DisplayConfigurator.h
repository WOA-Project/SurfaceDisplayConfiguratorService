#pragma once

#include <Windows.h>

DWORD WINAPI SetCorrectDisplayConfiguration();
BOOL WINAPI AreDisplaysAlreadyConfigured();
BOOL WINAPI MarkDisplaysAlreadyConfigured();