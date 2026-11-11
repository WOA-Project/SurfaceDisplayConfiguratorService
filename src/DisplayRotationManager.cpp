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

//
// Subject: Gets a display device
//
// Parameters:
//
//			   PanelId: The Panel ID (0, 1, etc)
//
//             DisplayDevice: The corresponding display device
//
// Returns: ERROR_SUCCESS if successful
//
HRESULT WINAPI
GetDisplayDeviceById(DWORD PanelId, PDISPLAY_DEVICE DisplayDevice, PDISPLAY_DEVICE DisplayDevice2)
{
    RtlZeroMemory(DisplayDevice, sizeof(DISPLAY_DEVICE));
    RtlZeroMemory(DisplayDevice2, sizeof(DISPLAY_DEVICE));

    DisplayDevice->cb = sizeof(DISPLAY_DEVICE);
    DisplayDevice2->cb = sizeof(DISPLAY_DEVICE);

    if (!EnumDisplayDevices(NULL, PanelId, DisplayDevice, 0))
    {
        return ERROR_NOT_FOUND;
    }

    if (!EnumDisplayDevices(DisplayDevice->DeviceName, 0, DisplayDevice2, 0))
    {
        return ERROR_NOT_FOUND;
    }

    return ERROR_SUCCESS;
}

BOOLEAN WINAPI
IsDeviceBoundToPanelId(CONST WCHAR *DeviceName, CONST WCHAR *DevicePanelId)
{
    HDEVINFO devInfo;
    DWORD dwBuffersize;
    SP_DEVINFO_DATA devData{};
    DEVPROPTYPE devProptype;
    LPWSTR devBuffer;
    DWORD i = 0;
    SP_CLASSINSTALL_HEADER ciHeader{};
    SP_PROPCHANGE_PARAMS pcParams{};

    if ((devInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES)) == NULL)
    {
        return FALSE;
    }

    devData.cbSize = sizeof(SP_DEVINFO_DATA);

    while (TRUE)
    {
        SetupDiEnumDeviceInfo(devInfo, i++, &devData);
        if (GetLastError() == ERROR_NO_MORE_ITEMS)
        {
            break;
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
            continue;
        }

        SIZE_T index = 0;
        SIZE_T length = wcslen(devBuffer);
        SIZE_T lengthDeviceName = wcslen(DeviceName);
        BOOLEAN match = FALSE;

        while (length > 0)
        {
            if (index >= dwBuffersize)
            {
                break;
            }

            match = memcmp(DeviceName, devBuffer + index, std::min(lengthDeviceName, length) * sizeof(WCHAR)) == 0;

            if (match)
            {
                break;
            }

            index += length + 1;

            if (index >= dwBuffersize)
            {
                break;
            }

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

        SIZE_T DeviceNameOffset = length + 1;

        SetupDiGetDeviceProperty(devInfo, &devData, &DEVPKEY_Device_Driver, &devProptype, NULL, 0, &dwBuffersize, 0);

        if ((devBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwBuffersize)) == NULL)
        {
            continue;
        }

        if (!SetupDiGetDeviceProperty(
                devInfo, &devData, &DEVPKEY_Device_Driver, &devProptype, (PBYTE)devBuffer, dwBuffersize, NULL, 0))
        {
            continue;
        }

        index = 0;
        length = wcslen(devBuffer);
        match = FALSE;

        while (length > 0)
        {
            if (index >= dwBuffersize)
            {
                break;
            }

            match = memcmp(
                        DeviceName + DeviceNameOffset,
                        devBuffer + index,
                        std::min(lengthDeviceName - DeviceNameOffset, length) * sizeof(WCHAR)) == 0;

            if (match)
            {
                break;
            }

            index += length + 1;

            if (index >= dwBuffersize)
            {
                break;
            }

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
            continue;
        }

        if (lstrcmp(devBuffer, DevicePanelId))
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

        if (!SetupDiDestroyDeviceInfoList(devInfo))
        {
            return FALSE;
        }

        return TRUE;
    }

    if (!SetupDiDestroyDeviceInfoList(devInfo))
    {
        return FALSE;
    }

    return FALSE;
}

//
// Subject: Gets a display device
//
// Parameters:
//
//			   DevicePanelId: The Panel Container Identifier
//
//             DisplayDevice: The corresponding display device
//
// Returns: ERROR_SUCCESS if successful
//
HRESULT WINAPI
GetDisplayDeviceByPanelId(CONST WCHAR *DevicePanelId, PDISPLAY_DEVICE DisplayDevice)
{
    DWORD i = 0;
    DISPLAY_DEVICE DisplayDevice2 = {0};

    while (SUCCEEDED(GetDisplayDeviceById(i++, DisplayDevice, &DisplayDevice2)))
    {
        if (IsDeviceBoundToPanelId(DisplayDevice2.DeviceID, DevicePanelId))
        {
            return ERROR_SUCCESS;
        }
    }

    return ERROR_NOT_FOUND;
}

HRESULT WINAPI
GetDisplayDeviceBestDisplayMode(PDISPLAY_DEVICE DisplayDevice, PDEVMODE deviceMode)
{
    DWORD i = 0;
    DWORD maxHeightSeen = 0;
    DEVMODE deviceMode2;

    RtlZeroMemory(deviceMode, sizeof(DEVMODE));
    deviceMode->dmSize = sizeof(DEVMODE);

    RtlZeroMemory(&deviceMode2, sizeof(DEVMODE));
    deviceMode2.dmSize = sizeof(DEVMODE);

    if (DisplayDevice->StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
    {
        if (!EnumDisplaySettings(DisplayDevice->DeviceName, ENUM_CURRENT_SETTINGS, deviceMode))
        {
            return ERROR_NOT_FOUND;
        }

        return ERROR_SUCCESS;
    }
    else
    {
        while (TRUE)
        {
            if (!EnumDisplaySettings(DisplayDevice->DeviceName, i, &deviceMode2))
            {
                break;
            }

            if (deviceMode2.dmPelsHeight > maxHeightSeen)
            {
                maxHeightSeen = deviceMode2.dmPelsHeight;
                printf("dmFields=0x%08X\n", deviceMode2.dmFields);
                memcpy(deviceMode, &deviceMode2, sizeof(DEVMODE));
            }

            i++;
        }

        if (maxHeightSeen == 0)
        {
            return ERROR_NOT_FOUND;
        }

        return ERROR_SUCCESS;
    }
}

HRESULT WINAPI
GetDisplayDevicePLD(CONST WCHAR *DeviceName, PACPI_PLD_V2_BUFFER Pld)
{
    HDEVINFO DeviceInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA devData = {0};
    DEVPROPTYPE devProptype;

    if (!SetupDiOpenDeviceInfo(DeviceInfo, DeviceName, NULL, 0, NULL))
    {
        return ERROR_NOT_FOUND;
    }

    devData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiGetDeviceProperty(
            DeviceInfo,
            &devData,
            &DEVPKEY_Device_PhysicalDeviceLocation,
            &devProptype,
            (PBYTE)Pld,
            sizeof(ACPI_PLD_V2_BUFFER),
            NULL,
            0))
    {
        SetupDiDestroyDeviceInfoList(DeviceInfo);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SetupDiDestroyDeviceInfoList(DeviceInfo);

    return ERROR_SUCCESS;
}

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

    if ((devInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES)) == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    devData.cbSize = sizeof(SP_DEVINFO_DATA);

    while (TRUE)
    {
        SetupDiEnumDeviceInfo(devInfo, i++, &devData);
        if (GetLastError() == ERROR_NO_MORE_ITEMS)
        {
            break;
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

HRESULT WINAPI
SetDisplayStates(
    CONST WCHAR *DisplayPanelId1,
    CONST WCHAR *DisplayPanelId2,
    INT DisplayOrientation1,
    INT DisplayOrientation2,
    BOOLEAN DisplayState1,
    BOOLEAN DisplayState2)
{
    /*ACPI_PLD_V2_BUFFER PLD1 = {0};
    ACPI_PLD_V2_BUFFER PLD2 = {0};
    HRESULT Status = ERROR_SUCCESS;

    Status = GetDisplayDevicePLD(DisplayPanelId1, &PLD1);
    if (FAILED(Status))
    {
        return Status;
    }

    Status = GetDisplayDevicePLD(DisplayPanelId1, &PLD2);
    if (FAILED(Status))
    {
        return Status;
    }

    if (PLD1.CabinetNumber == PLD2.CabinetNumber || PLD1.CabinetNumber > 1 || PLD2.CabinetNumber > 1)
    {
        return STATUS_INVALID_PARAMETER;
    }*/

    BOOLEAN PLD1IsLeftDisplay = TRUE;  // PLD1.CabinetNumber == 1;
    BOOLEAN PLD2IsRightDisplay = TRUE; // PLD2.CabinetNumber == 0;

    CONST WCHAR *LeftDisplayPanelId = PLD1IsLeftDisplay ? DisplayPanelId1 : DisplayPanelId2;
    CONST WCHAR *RightDisplayPanelId = PLD2IsRightDisplay ? DisplayPanelId2 : DisplayPanelId1;
    INT LeftDisplayOrientation = PLD1IsLeftDisplay ? DisplayOrientation1 : DisplayOrientation2;
    INT RightDisplayOrientation = PLD2IsRightDisplay ? DisplayOrientation2 : DisplayOrientation1;
    BOOLEAN LeftDisplayState = PLD1IsLeftDisplay ? DisplayState1 : DisplayState2;
    BOOLEAN RightDisplayState = PLD2IsRightDisplay ? DisplayState2 : DisplayState1;

    return SetDisplayStatesInternal(
        LeftDisplayPanelId,
        RightDisplayPanelId,
        LeftDisplayOrientation,
        RightDisplayOrientation,
        LeftDisplayState,
        RightDisplayState);
}

HRESULT WINAPI
SetDisplayStatesInternal(
    CONST WCHAR *LeftDisplayPanelId,
    CONST WCHAR *RightDisplayPanelId,
    INT LeftDisplayOrientation,
    INT RightDisplayOrientation,
    BOOLEAN LeftDisplayState,
    BOOLEAN RightDisplayState)
{
    DISPLAY_DEVICE LeftDisplayDevice = {0};
    DISPLAY_DEVICE RightDisplayDevice = {0};
    DEVMODE LeftDevMode = {0};
    DEVMODE RightDevMode = {0};
    BOOLEAN lastLeftDisplayState = FALSE;
    BOOLEAN lastRightDisplayState = FALSE;
    HRESULT Status = ERROR_SUCCESS;

    /*BOOLEAN IsSingleScreen = (!LeftDisplayState && RightDisplayState) || (!LeftDisplayState && RightDisplayState);

    if (IsSingleScreen && (lastLeftDisplayState == LeftDisplayState) && (lastRightDisplayState == RightDisplayState))
    {
        goto exit;
    }*/

    wprintf(
        L"SetDisplayStatesInternal Entry: LeftPanel='%s' RightPanel='%s' LeftOrientation=%d RightOrientation=%d LeftState=%d RightState=%d\n",
        LeftDisplayPanelId ? LeftDisplayPanelId : L"(null)",
        RightDisplayPanelId ? RightDisplayPanelId : L"(null)",
        LeftDisplayOrientation,
        RightDisplayOrientation,
        LeftDisplayState,
        RightDisplayState);

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

    if (LeftDisplayState == FALSE && lastLeftDisplayState)
    {
        // First make sure matching sensors are off to avoid init issues
        Status = SetHardwareEnabledStateForPanel(LeftDisplayPanelId, _T("HID_DEVICE_UP:000D_U:000F"), FALSE);
        if (FAILED(Status))
        {
            // goto exit;
            //  Non fatal for now
            Status = ERROR_SUCCESS;
        }
    }

    if (RightDisplayState == FALSE && lastRightDisplayState)
    {
        // First make sure matching sensors are off to avoid init issues
        Status = SetHardwareEnabledStateForPanel(RightDisplayPanelId, _T("HID_DEVICE_UP:000D_U:000F"), FALSE);
        if (FAILED(Status))
        {
            // goto exit;
            //  Non fatal for now
            Status = ERROR_SUCCESS;
        }
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
        switch (LeftDisplayOrientation)
        {
        case DMDO_DEFAULT: {
            //
            // X---- | Y----
            // | L | | | R |
            // ----- | -----
            //
            // X: (0,0); Y: (1350,0)
            LeftDevMode.dmPosition.x = 0;
            LeftDevMode.dmPosition.y = 0;
            RightDevMode.dmPosition.x = LeftDevMode.dmPelsWidth;
            RightDevMode.dmPosition.y = 0;
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
            LeftDevMode.dmPosition.x = 0;
            LeftDevMode.dmPosition.y = RightDevMode.dmPelsHeight;
            RightDevMode.dmPosition.x = 0;
            RightDevMode.dmPosition.y = 0;
            break;
        }
        case DMDO_180: {
            //
            // Y---- | X----
            // | R | | | L |
            // ----- | -----
            //
            // X: (1350,0); Y: (0,0)
            LeftDevMode.dmPosition.x = RightDevMode.dmPelsWidth;
            LeftDevMode.dmPosition.y = 0;
            RightDevMode.dmPosition.x = 0;
            RightDevMode.dmPosition.y = 0;
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
            LeftDevMode.dmPosition.x = 0;
            LeftDevMode.dmPosition.y = 0;
            RightDevMode.dmPosition.x = 0;
            RightDevMode.dmPosition.y = LeftDevMode.dmPelsHeight;
            break;
        }
        }

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

        BOOLEAN IsLeftDisplayPrimary = LeftDevMode.dmPosition.x == 0 && LeftDevMode.dmPosition.y == 0;

        LONG rc = ChangeDisplaySettingsEx(
            IsLeftDisplayPrimary ? LeftDisplayDevice.DeviceName : RightDisplayDevice.DeviceName,
            IsLeftDisplayPrimary ? &LeftDevMode : &RightDevMode,
            NULL,
            CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY,
            NULL);

        printf("CDSEx rc=%ld\n", rc);

        if (rc != DISP_CHANGE_SUCCESSFUL)
        {
            Status = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            goto exit;
        }

        rc = ChangeDisplaySettingsEx(
            IsLeftDisplayPrimary ? RightDisplayDevice.DeviceName : LeftDisplayDevice.DeviceName,
            IsLeftDisplayPrimary ? &RightDevMode : &LeftDevMode,
            NULL,
            CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET,
            NULL);

        printf("CDSEx rc=%ld\n", rc);

        if (rc != DISP_CHANGE_SUCCESSFUL)
        {
            Status = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
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
            if (ChangeDisplaySettingsEx(
                    LeftDisplayDevice.DeviceName,
                    &LeftDevMode,
                    NULL,
                    CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY,
                    NULL) != DISP_CHANGE_SUCCESSFUL)
            {
                Status = HRESULT_FROM_WIN32(GetLastError());
                goto exit;
            }

            if (lastRightDisplayState)
            {
                RightDevMode.dmPelsWidth = 0;
                RightDevMode.dmPelsHeight = 0;

                if (ChangeDisplaySettingsEx(
                        RightDisplayDevice.DeviceName,
                        &RightDevMode,
                        NULL,
                        CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET,
                        NULL) != DISP_CHANGE_SUCCESSFUL)
                {
                    Status = HRESULT_FROM_WIN32(GetLastError());
                    goto exit;
                }
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
            if (ChangeDisplaySettingsEx(
                    RightDisplayDevice.DeviceName,
                    &RightDevMode,
                    NULL,
                    CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY,
                    NULL) != DISP_CHANGE_SUCCESSFUL)
            {
                Status = HRESULT_FROM_WIN32(GetLastError());
                goto exit;
            }

            if (lastLeftDisplayState)
            {
                LeftDevMode.dmPelsWidth = 0;
                LeftDevMode.dmPelsHeight = 0;

                if (ChangeDisplaySettingsEx(
                        LeftDisplayDevice.DeviceName,
                        &LeftDevMode,
                        NULL,
                        CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET,
                        NULL) != DISP_CHANGE_SUCCESSFUL)
                {
                    Status = HRESULT_FROM_WIN32(GetLastError());
                    goto exit;
                }
            }
        }
    }

    {
        // Commit changes now
        LONG rc = ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);

        printf("CDSEx rc=%ld\n", rc);

        if (rc != DISP_CHANGE_SUCCESSFUL)
        {
            Status = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            goto exit;
        }
    }

    // Wait a second to make sure the display configuration is switched
    Sleep(1000);

    // Need to fix work areas if multiple displays due to Windows Shy Taskbar bugs...
    if (LeftDisplayState && RightDisplayState)
    {
        if (!UpdateMonitorWorkAreas())
        {
            Status = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
    }

    // Display needs to be turned on but was not currently attached
    if (LeftDisplayState == TRUE && !lastLeftDisplayState)
    {
        // First make sure matching sensors are on to avoid init issues
        Status = SetHardwareEnabledStateForPanel(LeftDisplayPanelId, _T("HID_DEVICE_UP:000D_U:000F"), TRUE);
        if (FAILED(Status))
        {
            // goto exit;
            //  Non fatal for now
            Status = ERROR_SUCCESS;
        }
    }

    // Display needs to be turned on but was not currently attached
    if (RightDisplayState == TRUE && !lastRightDisplayState)
    {
        // First make sure matching sensors are on to avoid init issues
        Status = SetHardwareEnabledStateForPanel(RightDisplayPanelId, _T("HID_DEVICE_UP:000D_U:000F"), TRUE);
        if (FAILED(Status))
        {
            // goto exit;
            //  Non fatal for now
            Status = ERROR_SUCCESS;
        }
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