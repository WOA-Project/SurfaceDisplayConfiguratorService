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
#include "NtAlpc.h"
#include "DeviceProperties.h"
#include "DisplayRotationManager.h"

CRITICAL_SECTION g_AutoRotationCriticalSection;

//
// The handle to the auto rotation ALPC port
//
HANDLE PortHandle = NULL;

//
// Subject: Notify auto rotation with the following current auto rotation settings
//			using ALPC port
//
// Parameters:
//
//			   Handle: the ALPC Port Handle
//
//             Orientation:
//             - DMDO_270     (Portrait)
//             - DMDO_90      (Portrait flipped)
//             - DMDO_180     (Landscape)
//             - DMDO_DEFAULT (Landscape flipped)
//
// Returns: HRESULT
//
HRESULT
NotifyAutoRotationAlpcPortOfOrientationChange(INT Orientation)
{
    ROTATION_COMMAND_MESSAGE RotationCommandMessage;
    HRESULT Status;

    if (PortHandle == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&RotationCommandMessage, sizeof(RotationCommandMessage));

    RotationCommandMessage.PortMessage.u1.s1.DataLength = sizeof(RotationCommandMessage.RotationMessage);
    RotationCommandMessage.PortMessage.u1.s1.TotalLength = sizeof(RotationCommandMessage);
    RotationCommandMessage.PortMessage.u2.s2.Type = 1;

    RotationCommandMessage.RotationMessage.Type = 2;
    RotationCommandMessage.RotationMessage.Orientation = Orientation;

    Status = NtAlpcSendWaitReceivePort(PortHandle, 0, (PVOID)&RotationCommandMessage, NULL, NULL, NULL, NULL, NULL);

    return Status;
}

BOOLEAN IsDisplay1SingleScreenFavorite = FALSE;

VOID WINAPI
ToggleFavoriteSingleScreenDisplay()
{
    EnterCriticalSection(&g_AutoRotationCriticalSection);

    IsDisplay1SingleScreenFavorite = IsDisplay1SingleScreenFavorite ? FALSE : TRUE;

    LeaveCriticalSection(&g_AutoRotationCriticalSection);
}

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
            match = memcmp(DeviceName, devBuffer + index, min(lengthDeviceName, length) * sizeof(WCHAR)) == 0;

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
            match = memcmp(
                        DeviceName + DeviceNameOffset,
                        devBuffer + index,
                        min(lengthDeviceName - DeviceNameOffset, length) * sizeof(WCHAR)) == 0;

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
// Subject: Sets a device enablement state based on its panel id attachement
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

INT WINAPI
ConvertSimpleOrientationToDMDO(SimpleOrientation orientation)
{
    switch (orientation)
    {
    case SimpleOrientation::NotRotated: {
        return DMDO_DEFAULT;
    }
    case SimpleOrientation::Rotated90DegreesCounterclockwise: {
        return DMDO_270;
    }
    case SimpleOrientation::Rotated180DegreesCounterclockwise: {
        return DMDO_180;
    }
    case SimpleOrientation::Rotated270DegreesCounterclockwise: {
        return DMDO_90;
    }
    }

    return DMDO_DEFAULT;
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
    DISPLAY_DEVICE DisplayDevice1 = {0};
    DISPLAY_DEVICE DisplayDevice2 = {0};
    DEVMODE DevMode1 = {0};
    DEVMODE DevMode2 = {0};
    HRESULT Status = ERROR_SUCCESS;

    EnterCriticalSection(&g_AutoRotationCriticalSection);

    Status = GetDisplayDeviceByPanelId(DisplayPanelId1, &DisplayDevice1);
    if (FAILED(Status))
    {
        goto exit;
    }

    Status = GetDisplayDeviceByPanelId(DisplayPanelId2, &DisplayDevice2);
    if (FAILED(Status))
    {
        goto exit;
    }

    Status = GetDisplayDeviceBestDisplayMode(&DisplayDevice1, &DevMode1);
    if (FAILED(Status))
    {
        goto exit;
    }

    Status = GetDisplayDeviceBestDisplayMode(&DisplayDevice2, &DevMode2);
    if (FAILED(Status))
    {
        goto exit;
    }

    if (DisplayState1 == FALSE) //&& (DisplayDevice1.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
        // First make sure matching sensors are off to avoid init issues
        Status = SetHardwareEnabledStateForPanel(DisplayPanelId1, L"HID_DEVICE_UP:000D_U:000F", FALSE);
        if (FAILED(Status))
        {
            goto exit;
        }
    }

    if (DisplayState2 == FALSE) //&& (DisplayDevice2.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
        // First make sure matching sensors are off to avoid init issues
        Status = SetHardwareEnabledStateForPanel(DisplayPanelId2, L"HID_DEVICE_UP:000D_U:000F", FALSE);
        if (FAILED(Status))
        {
            goto exit;
        }
    }

    // DevMode1.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION | DM_DISPLAYORIENTATION;
    DevMode1.dmPosition.x = 0;
    DevMode1.dmPosition.y = 0;

    // DevMode2.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION | DM_DISPLAYORIENTATION;
    DevMode2.dmPosition.x = 0;
    DevMode2.dmPosition.y = 0;

    //
    // In order to switch from portrait to landscape and vice versa we need to swap the resolution width and height
    // So we check for that
    //
    if ((DevMode1.dmDisplayOrientation + DisplayOrientation1) % 2 == 1)
    {
        INT temp = DevMode1.dmPelsHeight;
        DevMode1.dmPelsHeight = DevMode1.dmPelsWidth;
        DevMode1.dmPelsWidth = temp;
    }

    DevMode1.dmDisplayOrientation = DisplayOrientation1;

    if ((DevMode2.dmDisplayOrientation + DisplayOrientation2) % 2 == 1)
    {
        INT temp = DevMode2.dmPelsHeight;
        DevMode2.dmPelsHeight = DevMode2.dmPelsWidth;
        DevMode2.dmPelsWidth = temp;
    }

    DevMode2.dmDisplayOrientation = DisplayOrientation2;

    // Both displays on
    if (DisplayState1 && DisplayState2)
    {
        switch (DisplayOrientation1)
        {
        case DMDO_DEFAULT: {
            DevMode1.dmPosition.x = -1 * DevMode2.dmPelsWidth;
            break;
        }
        case DMDO_180: {
            DevMode2.dmPosition.x = -1 * DevMode1.dmPelsWidth;
            break;
        }
        case DMDO_90: {
            DevMode1.dmPosition.y = -1 * DevMode2.dmPelsHeight;
            break;
        }
        case DMDO_270: {
            DevMode2.dmPosition.y = -1 * DevMode1.dmPelsHeight;
            break;
        }
        }

        BOOLEAN IsDisplay1Primary = DevMode1.dmPosition.x == 0 && DevMode1.dmPosition.y == 0;

        if (ChangeDisplaySettingsEx(
                IsDisplay1Primary ? DisplayDevice1.DeviceName : DisplayDevice2.DeviceName,
                IsDisplay1Primary ? &DevMode1 : &DevMode2,
                NULL,
                CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY,
                NULL) != DISP_CHANGE_SUCCESSFUL)
        {
            Status = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        if (ChangeDisplaySettingsEx(
                IsDisplay1Primary ? DisplayDevice2.DeviceName : DisplayDevice1.DeviceName,
                IsDisplay1Primary ? &DevMode2 : &DevMode1,
                NULL,
                CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET,
                NULL) != DISP_CHANGE_SUCCESSFUL)
        {
            Status = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
    }
    // Left on, Right off
    else if (DisplayState1 && !DisplayState2)
    {
        DevMode2.dmPelsWidth = 0;
        DevMode2.dmPelsHeight = 0;

        if (ChangeDisplaySettingsEx(
                DisplayDevice2.DeviceName, &DevMode2, NULL, CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET, NULL) !=
            DISP_CHANGE_SUCCESSFUL)
        {
            Status = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        if (ChangeDisplaySettingsEx(
                DisplayDevice1.DeviceName,
                &DevMode1,
                NULL,
                CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY,
                NULL) != DISP_CHANGE_SUCCESSFUL)
        {
            Status = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
    }
    // Left off, Right on
    else if (!DisplayState1 && DisplayState2)
    {
        DevMode1.dmPelsWidth = 0;
        DevMode1.dmPelsHeight = 0;

        if (ChangeDisplaySettingsEx(
                DisplayDevice1.DeviceName, &DevMode1, NULL, CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET, NULL) !=
            DISP_CHANGE_SUCCESSFUL)
        {
            Status = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        if (ChangeDisplaySettingsEx(
                DisplayDevice2.DeviceName,
                &DevMode2,
                NULL,
                CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY,
                NULL) != DISP_CHANGE_SUCCESSFUL)
        {
            Status = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
    }

    // Commit changes now
    if (ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL) != DISP_CHANGE_SUCCESSFUL)
    {
        Status = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    // Display needs to be turned on but was not currently attached
    if (DisplayState1 == TRUE) //&& !(DisplayDevice1.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
        // First make sure matching sensors are on to avoid init issues
        Status = SetHardwareEnabledStateForPanel(DisplayPanelId1, L"HID_DEVICE_UP:000D_U:000F", TRUE);
        if (FAILED(Status))
        {
            goto exit;
        }
    }

    // Display needs to be turned on but was not currently attached
    if (DisplayState2 == TRUE) //&& !(DisplayDevice2.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
        // First make sure matching sensors are on to avoid init issues
        Status = SetHardwareEnabledStateForPanel(DisplayPanelId2, L"HID_DEVICE_UP:000D_U:000F", TRUE);
        if (FAILED(Status))
        {
            goto exit;
        }
    }

exit:
    LeaveCriticalSection(&g_AutoRotationCriticalSection);

    return Status;
}

HRESULT WINAPI
SetPanelsOrientationState(TwoPanelHingedDevicePostureReading reading)
{
    SimpleOrientation Panel1SimpleOrientation = reading.Panel1Orientation();
    SimpleOrientation Panel2SimpleOrientation = reading.Panel2Orientation();

    CONST WCHAR *panel1Id = reading.Panel1Id().c_str();
    CONST WCHAR *panel2Id = reading.Panel2Id().c_str();

    BOOLEAN Panel1UnknownOrientation =
        Panel1SimpleOrientation == SimpleOrientation::Faceup || Panel1SimpleOrientation == SimpleOrientation::Facedown;
    BOOLEAN Panel2UnknownOrientation =
        Panel2SimpleOrientation == SimpleOrientation::Faceup || Panel2SimpleOrientation == SimpleOrientation::Facedown;

    if (Panel1UnknownOrientation && !Panel2UnknownOrientation)
    {
        Panel1SimpleOrientation = Panel2SimpleOrientation;
    }
    else if (Panel2UnknownOrientation && !Panel1UnknownOrientation)
    {
        Panel2SimpleOrientation = Panel1SimpleOrientation;
    }

    INT Panel1Orientation = ConvertSimpleOrientationToDMDO(Panel1SimpleOrientation);
    INT Panel2Orientation = ConvertSimpleOrientationToDMDO(Panel2SimpleOrientation);

    if (reading.HingeState() == Windows::Internal::System::HingeState::Full)
    {
        return SetDisplayStates(
            panel1Id,
            panel2Id,
            Panel1Orientation,
            Panel2Orientation,
            IsDisplay1SingleScreenFavorite ? TRUE : FALSE,
            IsDisplay1SingleScreenFavorite ? FALSE : TRUE);
    }
    // All displays must be enabled
    else
    {
        return SetDisplayStates(panel1Id, panel2Id, Panel1Orientation, Panel2Orientation, TRUE, TRUE);
    }

    return ERROR_SUCCESS;
}

HRESULT WINAPI
SetExtendedDisplayConfiguration()
{
    return SetDisplayConfig(0, NULL, 0, NULL, SDC_APPLY | SDC_TOPOLOGY_EXTEND | SDC_PATH_PERSIST_IF_REQUIRED);
}

VOID
InitializeDisplayRotationManager()
{
    HRESULT Status;
    UNICODE_STRING DestinationString;
    OBJECT_ATTRIBUTES ObjectAttribs;
    ALPC_PORT_ATTRIBUTES PortAttribs = {0};

    //
    // Initialize ALPC Port. This will decide the way we do screen flip
    //
    RtlZeroMemory(&ObjectAttribs, sizeof(ObjectAttribs));
    ObjectAttribs.Length = sizeof(ObjectAttribs);

    RtlZeroMemory(&PortAttribs, sizeof(PortAttribs));
    PortAttribs.MaxMessageLength = 56;

    RtlInitUnicodeString(&DestinationString, L"\\RPC Control\\AutoRotationApiPort");

#pragma warning(disable : 6387)
    Status = NtAlpcConnectPort(
        &PortHandle,
        &DestinationString,
        &ObjectAttribs,
        &PortAttribs,
        ALPC_MSGFLG_SYNC_REQUEST,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
#pragma warning(default : 6387)

    if (FAILED(Status))
    {
        // Can't initialize critical section; skip Auto rotation API
        PortHandle = NULL;
    }

    InitializeCriticalSectionAndSpinCount(&g_AutoRotationCriticalSection, 0x00000400);
}