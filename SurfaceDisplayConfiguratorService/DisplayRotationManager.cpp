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

DWORD WINAPI
SetPanelOrientation(DWORD PanelId, INT Orientation)
{
    DISPLAY_DEVICE device = {0};
    DEVMODE deviceMode = {0};
    BOOLEAN IsPrimaryDisplay = FALSE;

    RtlZeroMemory(&device, sizeof(DISPLAY_DEVICE));
    RtlZeroMemory(&deviceMode, sizeof(DEVMODE));

    device.cb = sizeof(DISPLAY_DEVICE);

    if (!EnumDisplayDevices(NULL, PanelId, &device, 0))
    {
        return GetLastError();
    }

    if (!EnumDisplaySettings(device.DeviceName, ENUM_CURRENT_SETTINGS, &deviceMode))
    {
        return GetLastError();
    }

    //
    // In order to switch from portrait to landscape and vice versa we need to swap the resolution width and height
    // So we check for that
    //
    if ((deviceMode.dmDisplayOrientation + Orientation) % 2 == 1)
    {
        int temp = deviceMode.dmPelsHeight;
        deviceMode.dmPelsHeight = deviceMode.dmPelsWidth;
        deviceMode.dmPelsWidth = temp;
    }

    switch (Orientation)
    {
    case DMDO_DEFAULT: {
        deviceMode.dmPosition.x = (PanelId - 1) * deviceMode.dmPelsWidth;
        deviceMode.dmPosition.y = 0;
        IsPrimaryDisplay = PanelId == 1;
        break;
    }
    case DMDO_180: {
        deviceMode.dmPosition.x = -PanelId * deviceMode.dmPelsWidth;
        deviceMode.dmPosition.y = 0;
        IsPrimaryDisplay = PanelId == 0;
        break;
    }
    case DMDO_90: {
        deviceMode.dmPosition.x = 0;
        deviceMode.dmPosition.y = (PanelId - 1) * deviceMode.dmPelsHeight;
        IsPrimaryDisplay = PanelId == 1;
        break;
    }
    case DMDO_270: {
        deviceMode.dmPosition.x = 0;
        deviceMode.dmPosition.y = -PanelId * deviceMode.dmPelsHeight;
        IsPrimaryDisplay = PanelId == 0;
        break;
    }
    }

    //
    // Change the display orientation and save to the registry the changes (1 parameter for ChangeDisplaySettings)
    //
    if (deviceMode.dmFields | DM_DISPLAYORIENTATION)
    {
        deviceMode.dmDisplayOrientation = Orientation;

        DWORD flags = CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET;
        if (IsPrimaryDisplay)
        {
            flags |= CDS_SET_PRIMARY;
        }

        if (ChangeDisplaySettingsEx(device.DeviceName, &deviceMode, NULL, flags, NULL) != DISP_CHANGE_SUCCESSFUL)
        {
            return GetLastError();
        }
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI
CommitDisplaySettings()
{
    if (ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL) != DISP_CHANGE_SUCCESSFUL)
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
SetPanelsOrientationState(SimpleOrientation Panel1SimpleOrientation, SimpleOrientation Panel2SimpleOrientation)
{
    DWORD error = ERROR_SUCCESS;

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

    if ((error = SetPanelOrientation(0, Panel1Orientation)) != ERROR_SUCCESS)
    {
        return error;
    }

    if ((error = SetPanelOrientation(1, Panel2Orientation)) != ERROR_SUCCESS)
    {
        return error;
    }

    if ((error = CommitDisplaySettings()) != ERROR_SUCCESS)
    {
        return error;
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

DWORD WINAPI
SetCorrectDisplayConfiguration()
{
    DWORD error = ERROR_SUCCESS;

    if ((error = SetExtendedDisplayConfiguration()) != ERROR_SUCCESS)
    {
        return error;
    }

    if ((error = SetPanelsOrientationState(SimpleOrientation::NotRotated, SimpleOrientation::NotRotated)) !=
        ERROR_SUCCESS)
    {
        return error;
    }

    return ERROR_SUCCESS;
}