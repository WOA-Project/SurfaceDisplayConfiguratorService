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
#include <SetupAPI.h>
#include <Devpkey.h>
#include "DeviceProperties.h"
#include "AutoRotationApiPort.h"
#include "WorkAreas.h"
#include "DisplayRotationManager.h"
#include <tchar.h>
#include "DisplayTopology.h"
#include "DisplayMode.h"
#include "DisplayLayout.h"

#include "DisplayIdentityCache.h"

#include "DisplayActivation.h"
#include "DisplayConfiguration.h"

#include "BrightnessManager.h"
#include <OrientationManager.h>
#include <AutoRotateContext.h>
#include <AutoRotateService.h>
#include "DisplayValidation.h"
#include <AutoRotationPolicy.h>
#include <DisplaySynchronization.h>
#include <SdpPanelClient.h>
#include <InputStitching\InputSpacePolicy.h>
#include <InputStitching\InputSpaceConfiguration.h>
#include <InputStitching\InputSpaceRuntime.h>
#include <InputStitching\InputSpacePolicyAdapter.h>

//
// Subject: Sets a device enablement state based on its panel id attachment
//
// Parameters:
//
//			   devicePanelId: The Panel Container Identifier
//
//             deviceHardwareId: The corresponding compatible hardware id to toggle
//
//             enabledState: The enablement state for such device, if found
//
// Returns: ERROR_SUCCESS if successful
//
HRESULT WINAPI
SetHardwareEnabledStateForPanel(CONST WCHAR *devicePanelId, CONST WCHAR *deviceHardwareId, BOOLEAN enabledState)
{
    HDEVINFO devInfo;
    DWORD dwBuffersize;
    SP_DEVINFO_DATA devData{};
    DEVPROPTYPE devProptype;
    LPWSTR devBuffer;
    DWORD i = 0;
    SP_CLASSINSTALL_HEADER ciHeader{};
    SP_PROPCHANGE_PARAMS pcParams{};

    wprintf(L"%s %s\n", enabledState ? L"ENABLE" : L"DISABLE", devicePanelId);

    if ((devInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES)) == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    devData.cbSize = sizeof(SP_DEVINFO_DATA);

    while (TRUE)
    {
        if (!SetupDiEnumDeviceInfo(devInfo, i++, &devData))
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
            {
                break;
            }

            continue;
        }

        SetupDiGetDeviceProperty(
            devInfo, &devData, &DEVPKEY_Device_HardwareIds, &devProptype, NULL, 0, &dwBuffersize, 0);

        if ((devBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwBuffersize)) == NULL)
        {
            continue;
        }

        if (!SetupDiGetDeviceProperty(
                devInfo, &devData, &DEVPKEY_Device_HardwareIds, &devProptype, (PBYTE)devBuffer, dwBuffersize, NULL, 0))
        {
            HeapFree(GetProcessHeap(), 0, devBuffer);

            continue;
        }

        SIZE_T index = 0;
        SIZE_T length = wcslen(devBuffer);
        BOOLEAN match = FALSE;

        while (length > 0)
        {
            match = lstrcmp(devBuffer + index, deviceHardwareId) == 0;
            if (match)
            {
                break;
            }

            index += length + 1;
            length = wcslen(devBuffer + index);
        }

        if (!HeapFree(GetProcessHeap(), 0, devBuffer))
        {
            continue;
        }

        if (!match)
        {
            continue;
        }

        SetupDiGetDeviceProperty(devInfo, &devData, &DEVPKEY_Device_PanelId, &devProptype, NULL, 0, &dwBuffersize, 0);

        if ((devBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwBuffersize)) == NULL)
        {
            continue;
        }

        if (!SetupDiGetDeviceProperty(
                devInfo, &devData, &DEVPKEY_Device_PanelId, &devProptype, (PBYTE)devBuffer, dwBuffersize, NULL, 0))
        {
            HeapFree(GetProcessHeap(), 0, devBuffer);

            continue;
        }

        if (lstrcmp(devBuffer, devicePanelId))
        {
            if (!HeapFree(GetProcessHeap(), 0, devBuffer))
            {
                continue;
            }

            continue;
        }

        if (!HeapFree(GetProcessHeap(), 0, devBuffer))
        {
            continue;
        }

        ciHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        ciHeader.InstallFunction = DIF_PROPERTYCHANGE;
        pcParams.ClassInstallHeader = ciHeader;
        pcParams.StateChange = enabledState ? DICS_ENABLE : DICS_DISABLE;
        pcParams.Scope = DICS_FLAG_GLOBAL;
        pcParams.HwProfile = 0;

        if (!SetupDiSetClassInstallParams(
                devInfo, &devData, (PSP_CLASSINSTALL_HEADER)&pcParams, sizeof(SP_PROPCHANGE_PARAMS)))
        {
            continue;
        }

        wprintf(
            L"HW %s Panel=%s Hardware=%s\n", enabledState ? L"ENABLE" : L"DISABLE", devicePanelId, deviceHardwareId);

        if (!SetupDiChangeState(devInfo, &devData))
        {
            continue;
        }
    }

    if (!SetupDiDestroyDeviceInfoList(devInfo))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return ERROR_SUCCESS;
}

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
        HRESULT inputHr = InputStitching::g_InputSpaceRuntime.ApplyPolicy(inputPolicy);

        if (FAILED(inputHr))
        {
            wprintf(
                L"InputSpace policy application "
                L"failed: 0x%08X\n",
                inputHr);
        }
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

void
SetBrightnessViaDriverPath(UCHAR level)
{
    SdpPanelClient panelClient;
    if (!panelClient.Open())
    {
        wprintf(L"Failed to open SDP panel device.\n");
        return;
    }

    if (!panelClient.SetBrightness(SdpPanelClient::Primary, level))
    {
        wprintf(L"Failed to set primary brightness.\n");
    }

    if (!panelClient.SetBrightness(SdpPanelClient::Secondary, level))
    {
        wprintf(L"Failed to set secondary brightness.\n");
    }
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
    BrightnessManager brightnessMgr;
    BOOL brightnessMgrInitialized = FALSE;
    BYTE brightnessValue = 0;
    BOOLEAN NoOp = FALSE;
    BOOLEAN ConfigurationChanged = FALSE;

    HRESULT Status = ERROR_SUCCESS;

    struct DisplayLockGuard
    {
        DisplayLockGuard() { EnterCriticalSection(&g_DisplayOperationLock); }
        ~DisplayLockGuard() { LeaveCriticalSection(&g_DisplayOperationLock); }
    } Guard;

    ValidationSuppressionGuard Suppression;

    static std::atomic_uint64_t Counter{};

    wprintf(L"SetDisplayStatesInternal #%llu\n", ++Counter);

    g_LastPolicyEvaluationTick.store(GetTickCount64());

    if (!LeftDisplayState && !RightDisplayState)
    {
        wprintf(L"ERROR: Both displays requested off\n");
        return E_INVALIDARG;
    }

    wprintf(
        L"SetDisplayStatesInternal Entry: LeftPanel='%s' RightPanel='%s' LeftOrientation=%d RightOrientation=%d LeftState=%d RightState=%d\n",
        LeftDisplayPanelId ? LeftDisplayPanelId : L"(null)",
        RightDisplayPanelId ? RightDisplayPanelId : L"(null)",
        LeftDisplayOrientation,
        RightDisplayOrientation,
        LeftDisplayState,
        RightDisplayState);

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

    brightnessMgrInitialized = brightnessMgr.Initialize();

    if (brightnessMgrInitialized)
    {
        if (!brightnessMgr.GetBrightness(LeftDisplayDevice.DeviceName, brightnessValue) || brightnessValue == 0)
        {
            brightnessMgr.GetBrightness(RightDisplayDevice.DeviceName, brightnessValue);
            if (brightnessValue == 0)
            {
                brightnessValue = 50; // Default
            }
        }
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

    wprintf(
        L"Current: L=%d R=%d  Desired: L=%d R=%d\n",
        lastLeftDisplayState,
        lastRightDisplayState,
        LeftDisplayState,
        RightDisplayState);

    wprintf(L"ACTUAL: L=%d R=%d\n", lastLeftDisplayState, lastRightDisplayState);

    wprintf(L"LEFT Device=%s Flags=0x%08X\n", LeftDisplayDevice.DeviceName, LeftDisplayDevice.StateFlags);
    wprintf(L"RIGHT Device=%s Flags=0x%08X\n", RightDisplayDevice.DeviceName, RightDisplayDevice.StateFlags);

    wprintf(
        L"SYNC HW START: Desired L=%d R=%d Actual L=%d R=%d\n",
        LeftDisplayState,
        RightDisplayState,
        lastLeftDisplayState,
        lastRightDisplayState);

    Status = SynchronizePanelHardwareState(
        LeftDisplayPanelId,
        RightDisplayPanelId,
        LeftDisplayState,
        RightDisplayState,
        lastLeftDisplayState,
        lastRightDisplayState);

    wprintf(L"SYNC HW DONE hr=0x%08X\n", Status);

    if (FAILED(Status))
    {
        goto exit;
    }

    VerifyPreDisplayConfiguration(LeftDisplayPanelId, RightDisplayPanelId);

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

        wprintf(
            L"ORIENTATION PAIR: "
            L"L=%u R=%u\n",
            LeftDisplayOrientation,
            RightDisplayOrientation);

        BuildExtendedDisplayLayout(Connection, &LeftDevMode, &RightDevMode);

        wprintf(
            L"TOPOLOGY: "
            L"L=(%ld,%ld %lux%lu) "
            L"R=(%ld,%ld %lux%lu)\n",
            LeftDevMode.dmPosition.x,
            LeftDevMode.dmPosition.y,
            LeftDevMode.dmPelsWidth,
            LeftDevMode.dmPelsHeight,
            RightDevMode.dmPosition.x,
            RightDevMode.dmPosition.y,
            RightDevMode.dmPelsWidth,
            RightDevMode.dmPelsHeight);

        ApplyPrimaryOverride(PrimaryOverride, &LeftDevMode, &RightDevMode);

        wprintf(
            L"=====================================================\n"
            L"TOPOLOGY AFTER OVERRIDE:\n"
            L"LEFT [%s]\n"
            L"  fields=0x%08X\n"
            L"  pos=(%ld,%ld)\n"
            L"  res=%lux%lu\n"
            L"  orient=%lu\n"
            L"  freq=%lu\n"
            L"  bpp=%lu\n"
            L"  flags=0x%08X\n"
            L"\n"
            L"RIGHT [%s]\n"
            L"  fields=0x%08X\n"
            L"  pos=(%ld,%ld)\n"
            L"  res=%lux%lu\n"
            L"  orient=%lu\n"
            L"  freq=%lu\n"
            L"  bpp=%lu\n"
            L"  flags=0x%08X\n"
            L"=====================================================\n",
            LeftDisplayDevice.DeviceName,
            LeftDevMode.dmFields,
            LeftDevMode.dmPosition.x,
            LeftDevMode.dmPosition.y,
            LeftDevMode.dmPelsWidth,
            LeftDevMode.dmPelsHeight,
            LeftDevMode.dmDisplayOrientation,
            LeftDevMode.dmDisplayFrequency,
            LeftDevMode.dmBitsPerPel,
            LeftDevMode.dmDisplayFlags,
            RightDisplayDevice.DeviceName,
            RightDevMode.dmFields,
            RightDevMode.dmPosition.x,
            RightDevMode.dmPosition.y,
            RightDevMode.dmPelsWidth,
            RightDevMode.dmPelsHeight,
            RightDevMode.dmDisplayOrientation,
            RightDevMode.dmDisplayFrequency,
            RightDevMode.dmBitsPerPel,
            RightDevMode.dmDisplayFlags);

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
            wprintf(
                L"No effective state change "
                L"(actual configuration already matches)\n");

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
                wprintf(
                    L"No effective state change "
                    L"(actual configuration already matches)\n");

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
                wprintf(
                    L"No effective state change "
                    L"(actual configuration already matches)\n");

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
        wprintf(
            L"PRE-COMMIT FLAGS: Left=0x%08X Right=0x%08X\n",
            LeftDisplayDevice.StateFlags,
            RightDisplayDevice.StateFlags);

        // Commit changes now
        Status = CommitDisplayConfiguration();

        if (FAILED(Status))
        {
            goto exit;
        }
    }

    if (VerifyDisplayConfiguration(
            LeftDisplayPanelId,
            RightDisplayPanelId,
            LeftDisplayDevice,
            RightDisplayDevice,
            LeftDisplayOrientation,
            RightDisplayOrientation,
            LeftDevMode,
            RightDevMode))
    {
        BOOLEAN Expected = FALSE;

        if (g_ImmediateRepairRunning.compare_exchange_strong(Expected, TRUE))
        {
            g_LastRepairTick.store(0);

            SaveDesiredStateLocked(
                FALSE,
                LeftDisplayPanelId,
                RightDisplayPanelId,
                LeftDisplayOrientation,
                RightDisplayOrientation,
                LeftDevMode,
                RightDevMode,
                LeftDisplayState,
                RightDisplayState,
                PrimaryOverride);

            struct AutoResetRepairGuard
            {
                ~AutoResetRepairGuard() { g_ImmediateRepairRunning.store(FALSE); }
            } Guard;

            RepairDisplayConfiguration();
        }
    }

    if (brightnessMgrInitialized && brightnessValue != 0)
    {
        brightnessMgr.SetBrightness(LeftDisplayDevice.DeviceName, brightnessValue);
        brightnessMgr.SetBrightness(RightDisplayDevice.DeviceName, brightnessValue);

        SetBrightnessViaDriverPath(brightnessValue);
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

    wprintf(L"RESTORE HW START\n");

    Status = RestorePanelHardwareState(
        LeftDisplayPanelId,
        RightDisplayPanelId,
        LeftDisplayState,
        RightDisplayState,
        lastLeftDisplayState,
        lastRightDisplayState);

    wprintf(L"RESTORE HW DONE hr=0x%08X\n", Status);

    if (FAILED(Status))
    {
        goto exit;
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

    if (NoOp)
    {
        wprintf(L"Display state unchanged\n");
    }

    return Status;
}

HRESULT WINAPI
SetExtendedDisplayConfiguration()
{
    printf("Set Extended Display Configuration...\n");
    HRESULT rc = SetDisplayConfig(0, NULL, 0, NULL, SDC_APPLY | SDC_TOPOLOGY_EXTEND | SDC_PATH_PERSIST_IF_REQUIRED);
    if (FAILED(rc))
    {
        printf("SetDisplayConfig failed with rc=%ld\n", rc);
    }
    return rc;
}
