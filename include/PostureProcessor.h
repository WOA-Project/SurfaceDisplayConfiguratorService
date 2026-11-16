#pragma once

#include "pch.h"
#include <atomic>

enum class SingleScreenState
{
    DualScreen,
    PreviewPending,
    SingleScreenCommitted
};

extern std::atomic<SingleScreenState> s_InSingleScreenMode;

HRESULT WINAPI
ApplyPostureReading(TwoPanelHingedDevicePostureReading Reading);

VOID WINAPI
ProcessCurrentPosture(TwoPanelHingedDevicePostureReading Reading);

VOID WINAPI
TogglePreferredSingleScreen(TwoPanelHingedDevicePostureReading Reading);