#include "pch.h"

#include <windows.h>

#include "DisplayConfiguration.h"

static HRESULT
ApplyDisplaySettings(PCWSTR DeviceName, PDEVMODE DevMode, DWORD Flags)
{
    wprintf(
        L"BUILD MODE: "
        L"DISPLAY=%ls "
        L"ORIENTATION=%u\n",
        DeviceName,
        DevMode->dmDisplayOrientation);

    LONG rc = ChangeDisplaySettingsEx(DeviceName, DevMode, NULL, Flags, NULL);

    printf("CDSEx rc=%ld\n", rc);

    if (rc != DISP_CHANGE_SUCCESSFUL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    return ERROR_SUCCESS;
}

static BOOLEAN
IsLeftDisplayPrimary(PDEVMODE LeftDevMode, PDEVMODE RightDevMode)
{
    LONGLONG LeftDistance = (LONGLONG)LeftDevMode->dmPosition.x * LeftDevMode->dmPosition.x +
                            (LONGLONG)LeftDevMode->dmPosition.y * LeftDevMode->dmPosition.y;

    LONGLONG RightDistance = (LONGLONG)RightDevMode->dmPosition.x * RightDevMode->dmPosition.x +
                             (LONGLONG)RightDevMode->dmPosition.y * RightDevMode->dmPosition.y;

    return LeftDistance < RightDistance;
}

HRESULT WINAPI
ApplyExtendedDisplayConfiguration(
    PDISPLAY_DEVICE LeftDisplayDevice,
    PDISPLAY_DEVICE RightDisplayDevice,
    PDEVMODE LeftDevMode,
    PDEVMODE RightDevMode)
{
    BOOLEAN IsLeftPrimary = IsLeftDisplayPrimary(LeftDevMode, RightDevMode);
    HRESULT Status;

    Status = ApplyDisplaySettings(
        IsLeftPrimary ? LeftDisplayDevice->DeviceName : RightDisplayDevice->DeviceName,
        IsLeftPrimary ? LeftDevMode : RightDevMode,
        CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY);

    if (FAILED(Status))
    {
        return Status;
    }

    Status = ApplyDisplaySettings(
        IsLeftPrimary ? RightDisplayDevice->DeviceName : LeftDisplayDevice->DeviceName,
        IsLeftPrimary ? RightDevMode : LeftDevMode,
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

    wprintf(
        L"SINGLE LEFT: "
        L"pos=(%ld,%ld) "
        L"res=%lux%lu "
        L"orient=%lu "
        L"fields=0x%08X\n",
        LeftDevMode->dmPosition.x,
        LeftDevMode->dmPosition.y,
        LeftDevMode->dmPelsWidth,
        LeftDevMode->dmPelsHeight,
        LeftDevMode->dmDisplayOrientation,
        LeftDevMode->dmFields);

    Status = ApplyDisplaySettings(
        LeftDisplayDevice->DeviceName, LeftDevMode, CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY);

    if (FAILED(Status))
    {
        return Status;
    }

    if (LastRightDisplayState)
    {
        wprintf(
            L"DISABLING RIGHT VIA 0x0 MODE: current=%lux%lu\n", RightDevMode->dmPelsWidth, RightDevMode->dmPelsHeight);

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

    wprintf(
        L"SINGLE RIGHT: "
        L"pos=(%ld,%ld) "
        L"res=%lux%lu "
        L"orient=%lu "
        L"fields=0x%08X\n",
        RightDevMode->dmPosition.x,
        RightDevMode->dmPosition.y,
        RightDevMode->dmPelsWidth,
        RightDevMode->dmPelsHeight,
        RightDevMode->dmDisplayOrientation,
        RightDevMode->dmFields);

    Status = ApplyDisplaySettings(
        RightDisplayDevice->DeviceName, RightDevMode, CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY);

    if (FAILED(Status))
    {
        return Status;
    }

    if (LastLeftDisplayState)
    {
        wprintf(L"DISABLING LEFT VIA 0x0 MODE: current=%lux%lu\n", LeftDevMode->dmPelsWidth, LeftDevMode->dmPelsHeight);

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