#include "pch.h"
#include <SetupAPI.h>
#include <Devpkey.h>
#include "DisplayRotationManager.h"

#define DEFINE_DEVPROPKEY2(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) \
    EXTERN_C \
    const DEVPROPKEY DECLSPEC_SELECTANY name = {{l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}, pid}

#ifndef DEVPKEY_Device_PanelId
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PanelId,
    0x8dbc9c86,
    0x97a9,
    0x4bff,
    0x9b,
    0xc6,
    0xbf,
    0xe9,
    0x5d,
    0x3e,
    0x6d,
    0xad,
    2); // DEVPROP_TYPE_STRING
#endif

#ifndef DEVPKEY_Device_PhysicalDeviceLocation
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PhysicalDeviceLocation,
    0x540b947e,
    0x8b40,
    0x45bc,
    0xa8,
    0xa2,
    0x6a,
    0x0b,
    0x89,
    0x4c,
    0xbd,
    0xa2,
    9); // DEVPROP_TYPE_BINARY
#endif

typedef struct _ACPI_PLD_V2_BUFFER
{
    UINT32 Revision : 7;
    UINT32 IgnoreColor : 1;
    UINT32 Color : 24;
    UINT32 Panel : 3;
    UINT32 CardCageNumber : 8;
    UINT32 Reference : 1;
    UINT32 Rotation : 4;
    UINT32 Order : 5;
    UINT32 Reserved : 4;
    USHORT VerticalOffset;
    USHORT HorizontalOffset;
} ACPI_PLD_V2_BUFFER, *PACPI_PLD_V2_BUFFER;

BOOLEAN IsDisplay1SingleScreenFavorite = FALSE;

VOID WINAPI
ToggleFavoriteSingleScreenDisplay()
{
    IsDisplay1SingleScreenFavorite = IsDisplay1SingleScreenFavorite ? FALSE : TRUE;
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
DWORD WINAPI
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

        DWORD index = 0;
        DWORD length = wcslen(devBuffer);
        DWORD lengthDeviceName = wcslen(DeviceName);
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

        DWORD DeviceNameOffset = length + 1;

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
DWORD WINAPI
GetDisplayDeviceByPanelId(CONST WCHAR* DevicePanelId, PDISPLAY_DEVICE DisplayDevice)
{
    DWORD i = 0;
    DISPLAY_DEVICE DisplayDevice2 = {0};

    while (GetDisplayDeviceById(i++, DisplayDevice, &DisplayDevice2) == ERROR_SUCCESS)
    {
        if (IsDeviceBoundToPanelId(DisplayDevice2.DeviceID, DevicePanelId))
        {
            return ERROR_SUCCESS;
        }
    }

    return ERROR_NOT_FOUND;
}

DWORD WINAPI
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

DWORD WINAPI
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
DWORD WINAPI
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
        return GetLastError();
    }

    devData.cbSize = sizeof(SP_DEVINFO_DATA);

    while (TRUE)
    {
        SetupDiEnumDeviceInfo(devInfo, i++, &devData);
        if (GetLastError() == ERROR_NO_MORE_ITEMS)
        {
            break;
        }


        SetupDiGetDeviceProperty(devInfo, &devData, &DEVPKEY_Device_HardwareIds, &devProptype, NULL, 0, &dwBuffersize, 0);

        if ((devBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwBuffersize)) == NULL)
        {
            continue;
        }

        if (!SetupDiGetDeviceProperty(devInfo, &devData, &DEVPKEY_Device_HardwareIds, &devProptype, (PBYTE)devBuffer, dwBuffersize, NULL, 0))
        {
            continue;
        }

        DWORD index = 0;
        DWORD length = wcslen(devBuffer);
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

        if (!SetupDiGetDeviceProperty(devInfo, &devData, &DEVPKEY_Device_PanelId, &devProptype, (PBYTE)devBuffer, dwBuffersize, NULL, 0))
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

        if (!SetupDiSetClassInstallParams(devInfo, &devData, (PSP_CLASSINSTALL_HEADER)&pcParams, sizeof(SP_PROPCHANGE_PARAMS)))
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
        return GetLastError();
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

DWORD WINAPI
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
    DWORD error = ERROR_SUCCESS;

    error = GetDisplayDeviceByPanelId(DisplayPanelId1, &DisplayDevice1);
    if (error != ERROR_SUCCESS)
    {
        return error;
    }

    error = GetDisplayDeviceByPanelId(DisplayPanelId2, &DisplayDevice2);
    if (error != ERROR_SUCCESS)
    {
        return error;
    }

    error = GetDisplayDeviceBestDisplayMode(&DisplayDevice1, &DevMode1);
    if (error != ERROR_SUCCESS)
    {
        return error;
    }

    error = GetDisplayDeviceBestDisplayMode(&DisplayDevice2, &DevMode2);
    if (error != ERROR_SUCCESS)
    {
        return error;
    }

    if (DisplayState1 == FALSE) //&& (DisplayDevice1.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
        // First make sure matching sensors are off to avoid init issues
        error = SetHardwareEnabledStateForPanel(DisplayPanelId1, L"HID_DEVICE_UP:000D_U:000F", FALSE);
        if (error != ERROR_SUCCESS)
        {
            return error;
        }
    }

    if (DisplayState2 == FALSE) //&& (DisplayDevice2.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
        // First make sure matching sensors are off to avoid init issues
        error = SetHardwareEnabledStateForPanel(DisplayPanelId2, L"HID_DEVICE_UP:000D_U:000F", FALSE);
        if (error != ERROR_SUCCESS)
        {
            return error;
        }
    }

    //DevMode1.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION | DM_DISPLAYORIENTATION;
    DevMode1.dmPosition.x = 0;
    DevMode1.dmPosition.y = 0;

    //DevMode2.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_POSITION | DM_DISPLAYORIENTATION;
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
            DevMode1.dmPosition.x = -DevMode2.dmPelsWidth;
            break;
        }
        case DMDO_180: {
            DevMode2.dmPosition.x = -DevMode1.dmPelsWidth;
            break;
        }
        case DMDO_90: {
            DevMode1.dmPosition.y = -DevMode2.dmPelsHeight;
            break;
        }
        case DMDO_270: {
            DevMode2.dmPosition.y = -DevMode1.dmPelsHeight;
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
            return GetLastError();
        }

        if (ChangeDisplaySettingsEx(
                IsDisplay1Primary ? DisplayDevice2.DeviceName : DisplayDevice1.DeviceName,
                IsDisplay1Primary ? &DevMode2 : &DevMode1,
                NULL,
                CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET,
                NULL) !=
            DISP_CHANGE_SUCCESSFUL)
        {
            return GetLastError();
        }
    }
    // Left on, Right off
    else if (DisplayState1 && !DisplayState2)
    {
        DevMode2.dmPelsWidth = 0;
        DevMode2.dmPelsHeight = 0;

        if (ChangeDisplaySettingsEx(
                DisplayDevice2.DeviceName,
                &DevMode2,
                NULL,
                CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET,
                NULL) !=
            DISP_CHANGE_SUCCESSFUL)
        {
            return GetLastError();
        }

        if (ChangeDisplaySettingsEx(
                DisplayDevice1.DeviceName,
                &DevMode1,
                NULL,
                CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY,
                NULL) !=
            DISP_CHANGE_SUCCESSFUL)
        {
            return GetLastError();
        }
    }
    // Left off, Right on
    else if (!DisplayState1 && DisplayState2)
    {
        DevMode1.dmPelsWidth = 0;
        DevMode1.dmPelsHeight = 0;
        
        if (ChangeDisplaySettingsEx(
                DisplayDevice1.DeviceName,
                &DevMode1,
                NULL,
                CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET,
                NULL) !=
            DISP_CHANGE_SUCCESSFUL)
        {
            return GetLastError();
        }

        if (ChangeDisplaySettingsEx(
                DisplayDevice2.DeviceName,
                &DevMode2,
                NULL,
                CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET | CDS_SET_PRIMARY,
                NULL) !=
            DISP_CHANGE_SUCCESSFUL)
        {
            return GetLastError();
        }
    }

    // Commit changes now
    if (ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL) != DISP_CHANGE_SUCCESSFUL)
    {
        return GetLastError();
    }

    // Display needs to be turned on but was not currently attached
    if (DisplayState1 == TRUE) //&& !(DisplayDevice1.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
        // First make sure matching sensors are on to avoid init issues
        error = SetHardwareEnabledStateForPanel(DisplayPanelId1, L"HID_DEVICE_UP:000D_U:000F", TRUE);
        if (error != ERROR_SUCCESS)
        {
            return error;
        }
    }

    // Display needs to be turned on but was not currently attached
    if (DisplayState2 == TRUE) //&& !(DisplayDevice2.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
    {
        // First make sure matching sensors are on to avoid init issues
        error = SetHardwareEnabledStateForPanel(DisplayPanelId2, L"HID_DEVICE_UP:000D_U:000F", TRUE);
        if (error != ERROR_SUCCESS)
        {
            return error;
        }
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI
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

DWORD WINAPI
SetExtendedDisplayConfiguration()
{
    if (SetDisplayConfig(0, NULL, 0, NULL, SDC_APPLY | SDC_TOPOLOGY_EXTEND | SDC_PATH_PERSIST_IF_REQUIRED) !=
        ERROR_SUCCESS)
    {
        return GetLastError();
    }
    return ERROR_SUCCESS;
}