#pragma once

#include "winrt.h"
#include <windows.h>

constexpr INT DMDO_UNCHANGED = -1;

INT WINAPI
ConvertSimpleOrientationToDMDO(SimpleOrientation Orientation);

VOID WINAPI
NormalizePanelOrientations(SimpleOrientation *Panel1Orientation, SimpleOrientation *Panel2Orientation);

VOID WINAPI
BuildDisplayOrientations(
    BOOLEAN AutoRotationEnabled,
    TwoPanelHingedDevicePostureReading Reading,
    INT *Panel1Orientation,
    INT *Panel2Orientation);

struct DisplayPolicy
{
    BOOLEAN Display1Enabled;
    BOOLEAN Display2Enabled;

    INT Panel1Orientation;
    INT Panel2Orientation;
};

DisplayPolicy
BuildDisplayPolicy(
    BOOLEAN AutoRotationEnabled,
    const TwoPanelHingedDevicePostureReading& Reading,
    BOOLEAN PreferredDisplay1);