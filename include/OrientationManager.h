#pragma once

#include <windows.h>

#include "pch.h"

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