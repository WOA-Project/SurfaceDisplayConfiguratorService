/*
 * Copyright (c) 2022-2023 The DuoWOA authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction.
 */

#include "pch.h"

#include <windows.h>

#include "DisplayMode.h"

HRESULT WINAPI
GetDisplayDeviceBestDisplayMode(PDISPLAY_DEVICE DisplayDevice, PDEVMODE DeviceMode)
{
    DWORD i = 0;
    DWORD MaxHeightSeen = 0;

    DEVMODE DeviceModeCandidate;

    RtlZeroMemory(DeviceMode, sizeof(DEVMODE));
    DeviceMode->dmSize = sizeof(DEVMODE);

    RtlZeroMemory(&DeviceModeCandidate, sizeof(DEVMODE));
    DeviceModeCandidate.dmSize = sizeof(DEVMODE);

    //
    // Monitor is already attached.
    // Use current configuration.
    //
    if (DisplayDevice->StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
    {
        if (!EnumDisplaySettings(DisplayDevice->DeviceName, ENUM_CURRENT_SETTINGS, DeviceMode))
        {
            return ERROR_NOT_FOUND;
        }

        return ERROR_SUCCESS;
    }

    //
    // Monitor is detached.
    // Find the highest resolution mode.
    //
    while (TRUE)
    {
        if (!EnumDisplaySettings(DisplayDevice->DeviceName, i, &DeviceModeCandidate))
        {
            break;
        }

        if (DeviceModeCandidate.dmPelsHeight > MaxHeightSeen)
        {
            MaxHeightSeen = DeviceModeCandidate.dmPelsHeight;

            printf("dmFields=0x%08X\n", DeviceModeCandidate.dmFields);

            memcpy(DeviceMode, &DeviceModeCandidate, sizeof(DEVMODE));
        }

        i++;
    }

    if (MaxHeightSeen == 0)
    {
        return ERROR_NOT_FOUND;
    }

    return ERROR_SUCCESS;
}