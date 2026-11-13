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

    return SetDisplayStatesInternal(
        Panel1IsLeft ? DisplayPanelId1 : DisplayPanelId2,
        Panel1IsLeft ? DisplayPanelId2 : DisplayPanelId1,
        Panel1IsLeft ? DisplayOrientation1 : DisplayOrientation2,
        Panel1IsLeft ? DisplayOrientation2 : DisplayOrientation1,
        Panel1IsLeft ? DisplayState1 : DisplayState2,
        Panel1IsLeft ? DisplayState2 : DisplayState1,
        PrimaryOverride);
}

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
    DISPLAY_DEVICE LeftVerify{};
    DISPLAY_DEVICE RightVerify{};
    DISPLAY_DEVICE VerifyLeft{};
    DISPLAY_DEVICE VerifyRight{};
    DISPLAY_DEVICE FinalLeft{};
    DISPLAY_DEVICE FinalRight{};
    BOOLEAN LeftAttached = FALSE;
    BOOLEAN RightAttached = FALSE;
    HRESULT Status = ERROR_SUCCESS;

    BOOLEAN IsSingleScreen = (LeftDisplayState != RightDisplayState);
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

    if (IsSingleScreen && lastLeftDisplayState == LeftDisplayState && lastRightDisplayState == RightDisplayState)
    {
        wprintf(L"Skipping redundant single-screen transition\n");
        goto exit;
    }

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

    wprintf(
        L"PRE-SWAP: LeftPanel=%s LeftDevice=%s RightPanel=%s RightDevice=%s\n",
        LeftDisplayPanelId,
        LeftDisplayDevice.DeviceName,
        RightDisplayPanelId,
        RightDisplayDevice.DeviceName);

    if (_wcsicmp(LeftDisplayDevice.DeviceName, L"\\\\.\\DISPLAY2") == 0)
    {
        wprintf(L"DISPLAY2 detected as left display, swapping left/right devices and modes\n");

        DISPLAY_DEVICE TempDisplayDevice;
        DEVMODE TempDevMode;

        TempDisplayDevice = LeftDisplayDevice;
        LeftDisplayDevice = RightDisplayDevice;
        RightDisplayDevice = TempDisplayDevice;

        TempDevMode = LeftDevMode;
        LeftDevMode = RightDevMode;
        RightDevMode = TempDevMode;

        INT TempOrientation = LeftDisplayOrientation;
        LeftDisplayOrientation = RightDisplayOrientation;
        RightDisplayOrientation = TempOrientation;

        BOOLEAN TempState = LeftDisplayState;
        LeftDisplayState = RightDisplayState;
        RightDisplayState = TempState;

        PCWSTR TempPanelId = LeftDisplayPanelId;
        LeftDisplayPanelId = RightDisplayPanelId;
        RightDisplayPanelId = TempPanelId;
    }

    wprintf(
        L"POST-SWAP: LeftPanel=%s LeftDevice=%s RightPanel=%s RightDevice=%s\n",
        LeftDisplayPanelId,
        LeftDisplayDevice.DeviceName,
        RightDisplayPanelId,
        RightDisplayDevice.DeviceName);

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

    VerifyLeft.cb = sizeof(DISPLAY_DEVICE);
    VerifyRight.cb = sizeof(DISPLAY_DEVICE);

    HRESULT VerifyStatus;
    VerifyStatus = GetDisplayDeviceByPanelId(LeftDisplayPanelId, &VerifyLeft);
    if (SUCCEEDED(VerifyStatus))
    {
        wprintf(
            L"AFTER HW SYNC LEFT: %s Flags=0x%08X Attached=%d\n",
            VerifyLeft.DeviceName,
            VerifyLeft.StateFlags,
            (VerifyLeft.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0);
    }

    VerifyStatus = GetDisplayDeviceByPanelId(RightDisplayPanelId, &VerifyRight);
    if (SUCCEEDED(VerifyStatus))
    {
        wprintf(
            L"AFTER HW SYNC RIGHT: %s Flags=0x%08X Attached=%d\n",
            VerifyRight.DeviceName,
            VerifyRight.StateFlags,
            (VerifyRight.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0);
    }

    if ((VerifyLeft.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0 &&
        (VerifyRight.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0)
    {
        wprintf(L"!!! BOTH DISPLAYS DETACHED AFTER HW SYNC !!!\n");
    }

    LeftDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION | DM_DISPLAYORIENTATION;
    LeftDevMode.dmPosition.x = 0;
    LeftDevMode.dmPosition.y = 0;

    RightDevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION | DM_DISPLAYORIENTATION;
    RightDevMode.dmPosition.x = 0;
    RightDevMode.dmPosition.y = 0;

    //
    // In order to switch from portrait to landscape and vice versa we need to swap the resolution width and height
    // So we check for that
    //
    if ((LeftDevMode.dmDisplayOrientation + LeftDisplayOrientation) % 2 == 1)
    {
        INT temp = LeftDevMode.dmPelsHeight;
        LeftDevMode.dmPelsHeight = LeftDevMode.dmPelsWidth;
        LeftDevMode.dmPelsWidth = temp;
    }

    LeftDevMode.dmDisplayOrientation = LeftDisplayOrientation;

    if ((RightDevMode.dmDisplayOrientation + RightDisplayOrientation) % 2 == 1)
    {
        INT temp = RightDevMode.dmPelsHeight;
        RightDevMode.dmPelsHeight = RightDevMode.dmPelsWidth;
        RightDevMode.dmPelsWidth = temp;
    }

    RightDevMode.dmDisplayOrientation = RightDisplayOrientation;

    // Both displays on
    if (LeftDisplayState && RightDisplayState)
    {
        // Orientation shouldn't differ between both displays unless we are in tent mode
        BuildExtendedDisplayLayout(LeftDisplayOrientation, &LeftDevMode, &RightDevMode);

        ApplyPrimaryOverride(PrimaryOverride, &LeftDevMode, &RightDevMode);

        wprintf(
            L"LEFTDEVMODE[%s]: fields=0x%08X pos=(%ld,%ld) res=%lux%lu orient=%lu freq=%lu bpp=%lu flags=0x%08X\n",
            LeftDisplayDevice.DeviceName,
            LeftDevMode.dmFields,
            LeftDevMode.dmPosition.x,
            LeftDevMode.dmPosition.y,
            LeftDevMode.dmPelsWidth,
            LeftDevMode.dmPelsHeight,
            LeftDevMode.dmDisplayOrientation,
            LeftDevMode.dmDisplayFrequency,
            LeftDevMode.dmBitsPerPel,
            LeftDevMode.dmDisplayFlags);

        wprintf(
            L"RIGHTDEVMODE[%s]: fields=0x%08X pos=(%ld,%ld) res=%lux%lu orient=%lu freq=%lu bpp=%lu flags=0x%08X\n",
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

        Status =
            ApplyExtendedDisplayConfiguration(&LeftDisplayDevice, &RightDisplayDevice, &LeftDevMode, &RightDevMode);

        if (FAILED(Status))
        {
            goto exit;
        }
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
            Status = ApplySingleLeftDisplayConfiguration(
                &LeftDisplayDevice, &RightDisplayDevice, &LeftDevMode, &RightDevMode, lastRightDisplayState);
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
            Status = ApplySingleRightDisplayConfiguration(
                &LeftDisplayDevice, &RightDisplayDevice, &LeftDevMode, &RightDevMode, lastLeftDisplayState);
        }
    }

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

    LeftVerify.cb = sizeof(DISPLAY_DEVICE);
    RightVerify.cb = sizeof(DISPLAY_DEVICE);

    GetDisplayDeviceByPanelId(LeftDisplayPanelId, &LeftVerify);
    GetDisplayDeviceByPanelId(RightDisplayPanelId, &RightVerify);

    wprintf(L"POST-COMMIT FLAGS: Left=0x%08X Right=0x%08X\n", LeftVerify.StateFlags, RightVerify.StateFlags);

    LeftAttached = (LeftVerify.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0;
    RightAttached = (RightVerify.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0;

    wprintf(L"POST-COMMIT ATTACHED: Left=%d Right=%d\n", LeftAttached, RightAttached);

    if (!LeftAttached && !RightAttached)
    {
        wprintf(L"!!! NO ACTIVE DISPLAYS AFTER COMMIT !!!\n");
    }

    wprintf(L"Sleeping after commit...\n");

    // Wait a second to make sure the display configuration is switched
    Sleep(1000);

    wprintf(L"Awake after sleep\n");

    FinalLeft.cb = sizeof(DISPLAY_DEVICE);
    FinalRight.cb = sizeof(DISPLAY_DEVICE);

    GetDisplayDeviceByPanelId(LeftDisplayPanelId, &FinalLeft);
    GetDisplayDeviceByPanelId(RightDisplayPanelId, &FinalRight);

    wprintf(L"AFTER SLEEP FLAGS: Left=0x%08X Right=0x%08X\n", FinalLeft.StateFlags, FinalRight.StateFlags);

    wprintf(
        L"AFTER SLEEP ATTACHED: Left=%d Right=%d\n",
        (FinalLeft.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0,
        (FinalRight.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0);

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