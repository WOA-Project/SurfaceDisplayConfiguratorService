#include "pch.h"

#include <tchar.h>

#include "DisplayActivation.h"
#include "DisplayRotationManager.h"

static constexpr PCWSTR c_TouchDigitizerHardwareId = _T("HID_DEVICE_UP:000D_U:000F");

HRESULT WINAPI
DisablePanelHardwareIfNeeded(CONST WCHAR *PanelId, BOOLEAN DesiredState, BOOLEAN CurrentState)
{
    if (DesiredState)
    {
        return ERROR_SUCCESS;
    }

    if (!CurrentState)
    {
        return ERROR_SUCCESS;
    }

    HRESULT Status = SetHardwareEnabledStateForPanel(PanelId, c_TouchDigitizerHardwareId, FALSE);

    if (FAILED(Status))
    {
        //
        // Existing behavior.
        //
        return ERROR_SUCCESS;
    }

    return Status;
}

HRESULT WINAPI
EnablePanelHardwareIfNeeded(CONST WCHAR *PanelId, BOOLEAN DesiredState, BOOLEAN CurrentState)
{
    if (!DesiredState)
    {
        return ERROR_SUCCESS;
    }

    if (CurrentState)
    {
        return ERROR_SUCCESS;
    }

    HRESULT Status = SetHardwareEnabledStateForPanel(PanelId, c_TouchDigitizerHardwareId, TRUE);

    if (FAILED(Status))
    {
        //
        // Existing behavior.
        //
        return ERROR_SUCCESS;
    }

    return Status;
}

HRESULT WINAPI
SynchronizePanelHardwareState(
    CONST WCHAR *LeftPanelId,
    CONST WCHAR *RightPanelId,
    BOOLEAN LeftDesiredState,
    BOOLEAN RightDesiredState,
    BOOLEAN LeftCurrentState,
    BOOLEAN RightCurrentState)
{
    HRESULT Status;

    wprintf(L"Synchronize: Left=%d Right=%d\n", LeftDesiredState, RightDesiredState);

    Status = DisablePanelHardwareIfNeeded(LeftPanelId, LeftDesiredState, LeftCurrentState);

    if (FAILED(Status))
    {
        return Status;
    }

    Status = DisablePanelHardwareIfNeeded(RightPanelId, RightDesiredState, RightCurrentState);

    return Status;
}

HRESULT WINAPI
RestorePanelHardwareState(
    CONST WCHAR *LeftPanelId,
    CONST WCHAR *RightPanelId,
    BOOLEAN LeftDesiredState,
    BOOLEAN RightDesiredState,
    BOOLEAN LeftCurrentState,
    BOOLEAN RightCurrentState)
{
    HRESULT Status;

    printf(
        "RestorePanelHardwareState ENTRY LeftDesired=%d RightDesired=%d LeftCurrent=%d RightCurrent=%d\n",
        LeftDesiredState,
        RightDesiredState,
        LeftCurrentState,
        RightCurrentState);

    Status = EnablePanelHardwareIfNeeded(LeftPanelId, LeftDesiredState, LeftCurrentState);

    if (FAILED(Status))
    {
        return Status;
    }

    Status = EnablePanelHardwareIfNeeded(RightPanelId, RightDesiredState, RightCurrentState);

    printf("RestorePanelHardwareState EXIT\n");

    return Status;
}