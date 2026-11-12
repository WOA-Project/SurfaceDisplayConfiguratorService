#include "pch.h"

#include <windows.h>

#include "DisplayConfiguration.h"

static HRESULT
ApplyDisplaySettings(PCWSTR DeviceName, PDEVMODE DevMode, DWORD Flags)
{
    LONG rc = ChangeDisplaySettingsEx(DeviceName, DevMode, NULL, Flags, NULL);

    printf("CDSEx rc=%ld\n", rc);

    if (rc != DISP_CHANGE_SUCCESSFUL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    return ERROR_SUCCESS;
}

HRESULT WINAPI
ApplyExtendedDisplayConfiguration(
    PDISPLAY_DEVICE LeftDisplayDevice,
    PDISPLAY_DEVICE RightDisplayDevice,
    PDEVMODE LeftDevMode,
    PDEVMODE RightDevMode)
{
    BOOLEAN IsLeftDisplayPrimary = LeftDevMode->dmPosition.x == 0 && LeftDevMode->dmPosition.y == 0;

    HRESULT Status;

    Status = ApplyDisplaySettings(
        IsLeftDisplayPrimary ? LeftDisplayDevice->DeviceName : RightDisplayDevice->DeviceName,

        IsLeftDisplayPrimary ? LeftDevMode : RightDevMode,

        CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY);

    if (FAILED(Status))
    {
        return Status;
    }

    Status = ApplyDisplaySettings(
        IsLeftDisplayPrimary ? RightDisplayDevice->DeviceName : LeftDisplayDevice->DeviceName,

        IsLeftDisplayPrimary ? RightDevMode : LeftDevMode,

        CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET);

    return Status;
}

HRESULT WINAPI
ApplySingleLeftDisplayConfiguration(
    PDISPLAY_DEVICE LeftDisplayDevice,
    PDISPLAY_DEVICE RightDisplayDevice,
    PDEVMODE LeftDevMode,
    PDEVMODE RightDevMode,
    BOOLEAN LastRightDisplayState)
{
    HRESULT Status;

    Status = ApplyDisplaySettings(
        LeftDisplayDevice->DeviceName, LeftDevMode, CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY);

    if (FAILED(Status))
    {
        return Status;
    }

    if (LastRightDisplayState)
    {
        RightDevMode->dmPelsWidth = 0;
        RightDevMode->dmPelsHeight = 0;

        Status = ApplyDisplaySettings(
            RightDisplayDevice->DeviceName, RightDevMode, CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET);

        if (FAILED(Status))
        {
            return Status;
        }
    }

    return ERROR_SUCCESS;
}

HRESULT WINAPI
ApplySingleRightDisplayConfiguration(
    PDISPLAY_DEVICE LeftDisplayDevice,
    PDISPLAY_DEVICE RightDisplayDevice,
    PDEVMODE LeftDevMode,
    PDEVMODE RightDevMode,
    BOOLEAN LastLeftDisplayState)
{
    HRESULT Status;

    Status = ApplyDisplaySettings(
        RightDisplayDevice->DeviceName, RightDevMode, CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY);

    if (FAILED(Status))
    {
        return Status;
    }

    if (LastLeftDisplayState)
    {
        LeftDevMode->dmPelsWidth = 0;
        LeftDevMode->dmPelsHeight = 0;

        Status = ApplyDisplaySettings(
            LeftDisplayDevice->DeviceName, LeftDevMode, CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET);

        if (FAILED(Status))
        {
            return Status;
        }
    }

    return ERROR_SUCCESS;
}

HRESULT WINAPI
CommitDisplayConfiguration()
{
    LONG rc = ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);

    printf("CDSEx rc=%ld\n", rc);

    if (rc != DISP_CHANGE_SUCCESSFUL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    return ERROR_SUCCESS;
}