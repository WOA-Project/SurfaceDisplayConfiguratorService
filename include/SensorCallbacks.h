#pragma once

#include "pch.h"
#include "winrt.h"

VOID
OnPostureChanged(
    TwoPanelHingedDevicePosture const &Sender,
    TwoPanelHingedDevicePostureReadingChangedEventArgs const &Args);

VOID
OnFlipSensorReadingChanged(FlipSensor const &Sender, FlipSensorReadingChangedEventArgs const &Args);