/*
 * Copyright (c) 2022-2023 The DuoWOA authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "pch.h"
#include "AutoRotationApiPort.h"
#include "WorkAreas.h"
#include "DisplayRotationManager.h"
#include "DisplayTopology.h"
#include "DisplayMode.h"
#include "DisplayLayout.h"

#include "DisplayIdentityCache.h"

#include "DisplayConfiguration.h"

#include <OrientationManager.h>
#include <AutoRotateContext.h>
#include <AutoRotateService.h>
#include "DisplayValidation.h"
#include <AutoRotationPolicy.h>
#include <DisplaySynchronization.h>
#include <InputStitching\InputSpacePolicy.h>
#include <InputStitching\InputSpaceConfiguration.h>
#include <InputStitching\InputSpaceRuntime.h>
#include <InputStitching\InputSpacePolicyAdapter.h>

static VOID
ApplyPrimaryOverride(DISPLAY_PRIMARY_OVERRIDE Override, PDEVMODE Left, PDEVMODE Right)
{
    if (Override == DisplayPrimaryAuto)
    {
        return;
    }

    PDEVMODE DesiredPrimary = Override == DisplayPrimary1 ? Left : Right;

    LONG OriginX = DesiredPrimary->dmPosition.x;
    LONG OriginY = DesiredPrimary->dmPosition.y;

    Left->dmPosition.x -= OriginX;
    Left->dmPosition.y -= OriginY;

    Right->dmPosition.x -= OriginX;
    Right->dmPosition.y -= OriginY;
}

HRESULT WINAPI
SetDisplayStates(
    CONST WCHAR *DisplayPanelId1,
    CONST WCHAR *DisplayPanelId2,
    INT DisplayOrientation1,
    INT DisplayOrientation2,
    BOOLEAN DisplayState1,
    BOOLEAN DisplayState2,
    DISPLAY_PRIMARY_OVERRIDE PrimaryOverride)
{
    const DisplayIdentity *Panel1 = g_DisplayIdentityCache.FindByPanelId(DisplayPanelId1);
    const DisplayIdentity *Panel2 = g_DisplayIdentityCache.FindByPanelId(DisplayPanelId2);

    if (!Panel1 || !Panel2)
    {
        return ERROR_NOT_FOUND;
    }

    BOOLEAN Panel1IsLeft = Panel1->IsLeft;

    if (PrimaryOverride != DISPLAY_PRIMARY_OVERRIDE::DisplayPrimaryAuto && !Panel1IsLeft)
    {
        if (PrimaryOverride == DISPLAY_PRIMARY_OVERRIDE::DisplayPrimary1)
        {
            PrimaryOverride = DISPLAY_PRIMARY_OVERRIDE::DisplayPrimary2;
        }
        else
        {
            PrimaryOverride = DISPLAY_PRIMARY_OVERRIDE::DisplayPrimary1;
        }
    }

    HRESULT status = SetDisplayStatesInternal(
        Panel1IsLeft ? DisplayPanelId1 : DisplayPanelId2,
        Panel1IsLeft ? DisplayPanelId2 : DisplayPanelId1,
        Panel1IsLeft ? DisplayOrientation1 : DisplayOrientation2,
        Panel1IsLeft ? DisplayOrientation2 : DisplayOrientation1,
        Panel1IsLeft ? DisplayState1 : DisplayState2,
        Panel1IsLeft ? DisplayState2 : DisplayState1,
        PrimaryOverride);

    if (SUCCEEDED(status))
    {
        DisplayPolicy Policy = {};
        Policy.Display1Enabled = Panel1IsLeft ? DisplayState1 : DisplayState2;
        Policy.Display2Enabled = Panel1IsLeft ? DisplayState2 : DisplayState1;
        Policy.Panel1Orientation = Panel1IsLeft ? DisplayOrientation1 : DisplayOrientation2;
        Policy.Panel2Orientation = Panel1IsLeft ? DisplayOrientation2 : DisplayOrientation1;

        InputStitching::InputSpacePolicy inputPolicy =
            InputStitching::BuildInputSpacePolicy(Policy, InputStitching::g_InputSpaceConfiguration);
        InputStitching::g_InputSpaceRuntime.ApplyPolicy(inputPolicy);
    }

    return status;
}

static VOID
PrepareDisplayModeForOrientation(PDEVMODE DevMode, INT TargetOrientation)
{
    bool CurrentPortrait = DevMode->dmDisplayOrientation == DMDO_90 || DevMode->dmDisplayOrientation == DMDO_270;
    bool TargetPortrait = TargetOrientation == DMDO_90 || TargetOrientation == DMDO_270;
    if (CurrentPortrait != TargetPortrait)
    {
        std::swap(DevMode->dmPelsWidth, DevMode->dmPelsHeight);
    }

    DevMode->dmDisplayOrientation = TargetOrientation;
    DevMode->dmFields |= DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT;
}

struct ValidationSuppressionGuard
{
    ValidationSuppressionGuard() { g_SuppressValidation.store(true); }
    ~ValidationSuppressionGuard() { g_SuppressValidation.store(false); }
};

HRESULT WINAPI
SetDisplayStatesInternal(
    CONST WCHAR *LeftDisplayPanelId,
    CONST WCHAR *RightDisplayPanelId,
    INT LeftDisplayOrientation,
    INT RightDisplayOrientation,
    BOOLEAN LeftDisplayState,
    BOOLEAN RightDisplayState,
    DISPLAY_PRIMARY_OVERRIDE PrimaryOverride)
{
    DISPLAY_DEVICE LeftDisplayDevice = {0};
    DISPLAY_DEVICE RightDisplayDevice = {0};
    DEVMODE LeftDevMode = {0};
    DEVMODE RightDevMode = {0};
    BOOLEAN lastLeftDisplayState = FALSE;
    BOOLEAN lastRightDisplayState = FALSE;
    BOOLEAN NoOp = FALSE;
    BOOLEAN ConfigurationChanged = FALSE;

    HRESULT Status = ERROR_SUCCESS;

    struct DisplayLockGuard
    {
        DisplayLockGuard() { EnterCriticalSection(&g_DisplayOperationLock); }
        ~DisplayLockGuard() { LeaveCriticalSection(&g_DisplayOperationLock); }
    } Guard;

    ValidationSuppressionGuard Suppression;

    g_LastPolicyEvaluationTick.store(GetTickCount64());

    if (!LeftDisplayState && !RightDisplayState)
    {
        return E_INVALIDARG;
    }

    const DisplayIdentity *LeftIdentity = g_DisplayIdentityCache.FindByPanelId(LeftDisplayPanelId);
    const DisplayIdentity *RightIdentity = g_DisplayIdentityCache.FindByPanelId(RightDisplayPanelId);

    if (!LeftIdentity || !RightIdentity)
    {
        Status = ERROR_NOT_FOUND;
        goto exit;
    }

    LeftDisplayDevice.cb = sizeof(DISPLAY_DEVICE);
    RightDisplayDevice.cb = sizeof(DISPLAY_DEVICE);

    Status = GetDisplayDeviceByPanelId(LeftDisplayPanelId, &LeftDisplayDevice);
    if (FAILED(Status))
    {
        goto exit;
    }

    Status = GetDisplayDeviceByPanelId(RightDisplayPanelId, &RightDisplayDevice);
    if (FAILED(Status))
    {
        goto exit;
    }

    Status = EnumDisplaySettingsExW(LeftDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &LeftDevMode, 0);
    if (FAILED(Status))
    {
        goto exit;
    }

    Status = EnumDisplaySettingsExW(RightDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &RightDevMode, 0);
    if (FAILED(Status))
    {
        goto exit;
    }

    Status = GetDisplayDeviceBestDisplayMode(&LeftDisplayDevice, &LeftDevMode);
    if (FAILED(Status))
    {
        goto exit;
    }

    Status = GetDisplayDeviceBestDisplayMode(&RightDisplayDevice, &RightDevMode);
    if (FAILED(Status))
    {
        goto exit;
    }

    if (LeftDisplayOrientation == DMDO_UNCHANGED)
    {
        if (g_DesiredState.Valid)
        {
            LeftDisplayOrientation = g_DesiredState.LeftOrientation;
        }
        else
        {
            LeftDisplayOrientation = LeftDevMode.dmDisplayOrientation;
        }
    }

    if (RightDisplayOrientation == DMDO_UNCHANGED)
    {
        if (g_DesiredState.Valid)
        {
            RightDisplayOrientation = g_DesiredState.RightOrientation;
        }
        else
        {
            RightDisplayOrientation = RightDevMode.dmDisplayOrientation;
        }
    }

    lastLeftDisplayState = (LeftDisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP);
    lastRightDisplayState = (RightDisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP);

    //
    // In order to switch from portrait to landscape and vice versa we need to swap the resolution width and height
    // So we check for that
    //
    PrepareDisplayModeForOrientation(&LeftDevMode, LeftDisplayOrientation);
    PrepareDisplayModeForOrientation(&RightDevMode, RightDisplayOrientation);

    LeftDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION | DM_DISPLAYORIENTATION;
    LeftDevMode.dmPosition.x = 0;
    LeftDevMode.dmPosition.y = 0;

    RightDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION | DM_DISPLAYORIENTATION;
    RightDevMode.dmPosition.x = 0;
    RightDevMode.dmPosition.y = 0;

    // Both displays on
    if (LeftDisplayState && RightDisplayState)
    {
        // Orientation shouldn't differ between both displays unless we are in tent mode
        DISPLAY_CONNECTION Connection = DetermineDisplayConnection(LeftDisplayOrientation, RightDisplayOrientation);
        BuildExtendedDisplayLayout(Connection, &LeftDevMode, &RightDevMode);

        ApplyPrimaryOverride(PrimaryOverride, &LeftDevMode, &RightDevMode);

        if (IsConfigurationSatisfied(
                LeftDisplayPanelId,
                RightDisplayPanelId,
                LeftDisplayOrientation,
                RightDisplayOrientation,
                LeftDisplayState,
                RightDisplayState,
                LeftDevMode.dmPosition.x,
                LeftDevMode.dmPosition.y,
                RightDevMode.dmPosition.x,
                RightDevMode.dmPosition.y,
                LeftDevMode.dmPelsWidth,
                LeftDevMode.dmPelsHeight,
                RightDevMode.dmPelsWidth,
                RightDevMode.dmPelsHeight))
        {
            NoOp = TRUE;
            Status = ERROR_SUCCESS;
            goto exit;
        }

        UpdateAutoRotationPolicy(LeftDisplayOrientation, RightDisplayOrientation, LeftDisplayState, RightDisplayState);

        Status =
            ApplyExtendedDisplayConfiguration(&LeftDisplayDevice, &RightDisplayDevice, &LeftDevMode, &RightDevMode);

        if (FAILED(Status))
        {
            goto exit;
        }

        ConfigurationChanged = TRUE;
    }
    // Left on, Right off
    else if (LeftDisplayState && !RightDisplayState)
    {
        if (lastLeftDisplayState && !lastRightDisplayState)
        {
            Status = NotifyAutoRotationAlpcPortOfOrientationChange(LeftDisplayOrientation);
            if (FAILED(Status))
            {
                goto exit;
            }
        }
        else
        {
            RightDevMode.dmSize = sizeof(DEVMODE);
            RightDevMode.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;

            RightDevMode.dmPosition.x = 0;
            RightDevMode.dmPosition.y = 0;
            RightDevMode.dmPelsWidth = 0;
            RightDevMode.dmPelsHeight = 0;

            if (IsConfigurationSatisfied(
                    LeftDisplayPanelId,
                    RightDisplayPanelId,
                    LeftDisplayOrientation,
                    RightDisplayOrientation,
                    LeftDisplayState,
                    RightDisplayState,
                    LeftDevMode.dmPosition.x,
                    LeftDevMode.dmPosition.y,
                    RightDevMode.dmPosition.x,
                    RightDevMode.dmPosition.y,
                    LeftDevMode.dmPelsWidth,
                    LeftDevMode.dmPelsHeight,
                    RightDevMode.dmPelsWidth,
                    RightDevMode.dmPelsHeight))
            {
                NoOp = TRUE;
                Status = ERROR_SUCCESS;
                goto exit;
            }

            Status = ApplySingleLeftDisplayConfiguration(
                &LeftDisplayDevice, &RightDisplayDevice, &LeftDevMode, &RightDevMode, lastRightDisplayState);

            if (SUCCEEDED(Status))
            {
                ConfigurationChanged = TRUE;
            }
        }
    }
    // Left off, Right on
    else if (!LeftDisplayState && RightDisplayState)
    {
        if (!lastLeftDisplayState && lastRightDisplayState)
        {
            Status = NotifyAutoRotationAlpcPortOfOrientationChange(RightDisplayOrientation);
            if (FAILED(Status))
            {
                goto exit;
            }
        }
        else
        {
            LeftDevMode.dmSize = sizeof(DEVMODE);
            LeftDevMode.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;

            LeftDevMode.dmPosition.x = 0;
            LeftDevMode.dmPosition.y = 0;
            LeftDevMode.dmPelsWidth = 0;
            LeftDevMode.dmPelsHeight = 0;

            if (IsConfigurationSatisfied(
                    LeftDisplayPanelId,
                    RightDisplayPanelId,
                    LeftDisplayOrientation,
                    RightDisplayOrientation,
                    LeftDisplayState,
                    RightDisplayState,
                    LeftDevMode.dmPosition.x,
                    LeftDevMode.dmPosition.y,
                    RightDevMode.dmPosition.x,
                    RightDevMode.dmPosition.y,
                    LeftDevMode.dmPelsWidth,
                    LeftDevMode.dmPelsHeight,
                    RightDevMode.dmPelsWidth,
                    RightDevMode.dmPelsHeight))
            {
                NoOp = TRUE;
                Status = ERROR_SUCCESS;
                goto exit;
            }

            Status = ApplySingleRightDisplayConfiguration(
                &LeftDisplayDevice, &RightDisplayDevice, &LeftDevMode, &RightDevMode, lastLeftDisplayState);

            if (SUCCEEDED(Status))
            {
                ConfigurationChanged = TRUE;
            }
        }
    }

    if (ConfigurationChanged)
    {
        // Commit changes now
        Status = CommitDisplayConfiguration();

        if (FAILED(Status))
        {
            goto exit;
        }
    }

    // Need to fix work areas if multiple displays due to Windows Shy Taskbar bugs...
    if (LeftDisplayState && RightDisplayState)
    {
        if (!UpdateMonitorWorkAreas())
        {
            Status = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
    }

exit:
    if (SUCCEEDED(Status))
    {
        SaveDesiredStateLocked(
            NoOp,
            LeftDisplayPanelId,
            RightDisplayPanelId,
            LeftDisplayOrientation,
            RightDisplayOrientation,
            LeftDevMode,
            RightDevMode,
            LeftDisplayState,
            RightDisplayState,
            PrimaryOverride);

        if (ConfigurationChanged)
        {
            g_LastRepairTick.store(GetTickCount64());
        }
    }

    return Status;
}

HRESULT WINAPI
SetExtendedDisplayConfiguration()
{
    return SetDisplayConfig(0, NULL, 0, NULL, SDC_APPLY | SDC_TOPOLOGY_EXTEND | SDC_PATH_PERSIST_IF_REQUIRED);
}
