#pragma once

#include "pch.h"

HRESULT WINAPI
ApplyPostureReading(TwoPanelHingedDevicePostureReading Reading);

VOID WINAPI
ProcessCurrentPosture(TwoPanelHingedDevicePostureReading Reading);

VOID WINAPI
TogglePreferredSingleScreen(TwoPanelHingedDevicePostureReading Reading);