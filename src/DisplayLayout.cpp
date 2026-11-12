/*
 * Copyright (c) 2022-2023 The DuoWOA authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction.
 */

#include "pch.h"

#include "DisplayLayout.h"

VOID WINAPI
BuildExtendedDisplayLayout(INT LeftDisplayOrientation, PDEVMODE LeftDevMode, PDEVMODE RightDevMode)
{
    switch (LeftDisplayOrientation)
    {
    case DMDO_DEFAULT: {
        //
        // X---- | Y----
        // | L | | | R |
        // ----- | -----
        //
        // X: (0,0); Y: (1350,0)
        //

        LeftDevMode->dmPosition.x = 0;
        LeftDevMode->dmPosition.y = 0;

        RightDevMode->dmPosition.x = LeftDevMode->dmPelsWidth;
        RightDevMode->dmPosition.y = 0;

        break;
    }

    case DMDO_90: {
        //
        //  +------
        //  | CCW
        //  v 90°
        //
        // Y-------|
        // |   R   |
        // |-------|
        // ---------
        // X-------|
        // |   L   |
        // |-------|
        //
        // X: (0,1350); Y: (0,0)
        //

        LeftDevMode->dmPosition.x = 0;
        LeftDevMode->dmPosition.y = RightDevMode->dmPelsHeight;

        RightDevMode->dmPosition.x = 0;
        RightDevMode->dmPosition.y = 0;

        break;
    }

    case DMDO_180: {
        //
        // Y---- | X----
        // | R | | | L |
        // ----- | -----
        //
        // X: (1350,0); Y: (0,0)
        //

        LeftDevMode->dmPosition.x = RightDevMode->dmPelsWidth;
        LeftDevMode->dmPosition.y = 0;

        RightDevMode->dmPosition.x = 0;
        RightDevMode->dmPosition.y = 0;

        break;
    }

    case DMDO_270: {
        //
        //   -----+
        //   CCW  |
        //   270° v
        //
        // X-------|
        // |   L   |
        // |-------|
        // ---------
        // Y-------|
        // |   R   |
        // |-------|
        //
        // X: (0,0); Y: (0,1350)
        //

        LeftDevMode->dmPosition.x = 0;
        LeftDevMode->dmPosition.y = 0;

        RightDevMode->dmPosition.x = 0;
        RightDevMode->dmPosition.y = LeftDevMode->dmPelsHeight;

        break;
    }

    default:
        break;
    }
}