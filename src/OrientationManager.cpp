/*
 * Copyright (c) 2022-2023 The DuoWOA authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction.
 */

#include "pch.h"

#include "winrt.h"
#include "OrientationManager.h"

static bool
TryConvertSimpleOrientationToDMDO(SimpleOrientation Orientation, INT *Dmdo)
{
    switch (Orientation)
    {
    case SimpleOrientation::NotRotated:
        *Dmdo = DMDO_DEFAULT;
        return true;

    case SimpleOrientation::Rotated90DegreesCounterclockwise:
        *Dmdo = DMDO_90;
        return true;

    case SimpleOrientation::Rotated180DegreesCounterclockwise:
        *Dmdo = DMDO_180;
        return true;

    case SimpleOrientation::Rotated270DegreesCounterclockwise:
        *Dmdo = DMDO_270;
        return true;

    default:
        return false;
    }
}

DisplayPolicy
BuildDisplayPolicy(
    BOOLEAN AutoRotationEnabled,
    const TwoPanelHingedDevicePostureReading &Reading,
    BOOLEAN PreferredDisplay1)
{
    DisplayPolicy Policy{};

    Policy.Display1Enabled = TRUE;
    Policy.Display2Enabled = TRUE;

    Policy.Panel1Orientation = DMDO_UNCHANGED;
    Policy.Panel2Orientation = DMDO_UNCHANGED;

    if (!AutoRotationEnabled)
    {
        return Policy;
    }

    const auto Hinge = Reading.HingeState();
    const auto P1 = Reading.Panel1Orientation();
    const auto P2 = Reading.Panel2Orientation();

    wprintf(L"Hinge=%u P1=%u P2=%u\n", (UINT)Hinge, (UINT)P1, (UINT)P2);

    INT Dmdo;

    if (TryConvertSimpleOrientationToDMDO(P1, &Dmdo))
    {
        Policy.Panel1Orientation = Dmdo;
    }

    if (TryConvertSimpleOrientationToDMDO(P2, &Dmdo))
    {
        Policy.Panel2Orientation = Dmdo;
    }

    if (Hinge == Windows::Internal::System::HingeState::Flat)
    {
        Policy.Display1Enabled = TRUE;
        Policy.Display2Enabled = TRUE;

        return Policy;
    }

    if (Hinge == Windows::Internal::System::HingeState::Full)
    {
        //
        // Tent
        //
        if ((P1 == SimpleOrientation::Rotated90DegreesCounterclockwise &&
             P2 == SimpleOrientation::Rotated270DegreesCounterclockwise) ||

            (P1 == SimpleOrientation::Rotated270DegreesCounterclockwise &&
             P2 == SimpleOrientation::Rotated90DegreesCounterclockwise))
        {
            Policy.Display1Enabled = TRUE;
            Policy.Display2Enabled = TRUE;

            return Policy;
        }

        //
        // Surface folded behind
        //
        if (P1 == SimpleOrientation::Faceup && P2 == SimpleOrientation::Facedown)
        {
            Policy.Display1Enabled = TRUE;
            Policy.Display2Enabled = FALSE;

            return Policy;
        }

        if (P1 == SimpleOrientation::Facedown && P2 == SimpleOrientation::Faceup)
        {
            Policy.Display1Enabled = FALSE;
            Policy.Display2Enabled = TRUE;

            return Policy;
        }

        //
        // Full + 0/0 or 180/180.
        //
        Policy.Display1Enabled = PreferredDisplay1;
        Policy.Display2Enabled = !PreferredDisplay1;

        return Policy;
    }

    if (Hinge == Windows::Internal::System::HingeState::Convex)
    {
        if (P1 == SimpleOrientation::Faceup && P2 == SimpleOrientation::Facedown)
        {
            Policy.Display1Enabled = TRUE;
            Policy.Display2Enabled = FALSE;

            return Policy;
        }

        if (P1 == SimpleOrientation::Facedown && P2 == SimpleOrientation::Faceup)
        {
            Policy.Display1Enabled = FALSE;
            Policy.Display2Enabled = TRUE;

            return Policy;
        }
    }

    return Policy;
}