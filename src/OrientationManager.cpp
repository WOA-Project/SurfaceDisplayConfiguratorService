/*
 * Copyright (c) 2022-2023 The DuoWOA authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction.
 */

#include "pch.h"

#include "OrientationManager.h"

INT WINAPI
ConvertSimpleOrientationToDMDO(SimpleOrientation Orientation)
{
    switch (Orientation)
    {
    case SimpleOrientation::NotRotated:
        return DMDO_DEFAULT;

    case SimpleOrientation::Rotated90DegreesCounterclockwise:
        return DMDO_90;

    case SimpleOrientation::Rotated180DegreesCounterclockwise:
        return DMDO_180;

    case SimpleOrientation::Rotated270DegreesCounterclockwise:
        return DMDO_270;

    default:
        break;
    }

    return DMDO_DEFAULT;
}

VOID WINAPI
NormalizePanelOrientations(SimpleOrientation *Panel1Orientation, SimpleOrientation *Panel2Orientation)
{
    BOOLEAN Panel1Unknown =
        *Panel1Orientation == SimpleOrientation::Faceup || *Panel1Orientation == SimpleOrientation::Facedown;

    BOOLEAN Panel2Unknown =
        *Panel2Orientation == SimpleOrientation::Faceup || *Panel2Orientation == SimpleOrientation::Facedown;

    if (Panel1Unknown && !Panel2Unknown)
    {
        *Panel1Orientation = *Panel2Orientation;
    }
    else if (Panel2Unknown && !Panel1Unknown)
    {
        *Panel2Orientation = *Panel1Orientation;
    }
}

VOID WINAPI
BuildDisplayOrientations(
    BOOLEAN AutoRotationEnabled,
    TwoPanelHingedDevicePostureReading Reading,
    INT *Panel1Orientation,
    INT *Panel2Orientation)
{
    if (!Panel1Orientation || !Panel2Orientation)
    {
        return;
    }

    if (!AutoRotationEnabled)
    {
        return;
    }

    SimpleOrientation Panel1SimpleOrientation = Reading.Panel1Orientation();

    SimpleOrientation Panel2SimpleOrientation = Reading.Panel2Orientation();

    NormalizePanelOrientations(&Panel1SimpleOrientation, &Panel2SimpleOrientation);

    *Panel1Orientation = ConvertSimpleOrientationToDMDO(Panel1SimpleOrientation);

    *Panel2Orientation = ConvertSimpleOrientationToDMDO(Panel2SimpleOrientation);
}