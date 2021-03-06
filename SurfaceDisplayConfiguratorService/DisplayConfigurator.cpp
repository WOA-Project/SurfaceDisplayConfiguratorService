#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>
#include <initguid.h>
#include <Devpkey.h>
#include <SetupAPI.h>
#include <strsafe.h>
#include <cfgmgr32.h>

DWORD WINAPI SetExtendedDisplayConfiguration()
{
    if (SetDisplayConfig(
        0,
        NULL,
        0,
        NULL,
        SDC_APPLY | SDC_TOPOLOGY_EXTEND | SDC_PATH_PERSIST_IF_REQUIRED
    ) != ERROR_SUCCESS)
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI SetLeftPanelPosition()
{
    DISPLAY_DEVICE device = { 0 };
    DEVMODE deviceMode = { 0 };

    RtlZeroMemory(&device, sizeof(DISPLAY_DEVICE));
    RtlZeroMemory(&deviceMode, sizeof(DEVMODE));

    device.cb = sizeof(DISPLAY_DEVICE);

    if (!EnumDisplayDevices(NULL, 0, &device, 0))
    {
        return GetLastError();
    }

    if (!EnumDisplaySettings(device.DeviceName, -1, &deviceMode))
    {
        return GetLastError();
    }

    deviceMode.dmPosition.x = 0;
    deviceMode.dmPosition.y = 0;

    if (ChangeDisplaySettingsEx(
        device.DeviceName,
        &deviceMode,
        NULL,
        CDS_SET_PRIMARY | CDS_UPDATEREGISTRY | CDS_NORESET,
        NULL
    ) != DISP_CHANGE_SUCCESSFUL)
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI SetRightPanelPosition()
{
    DISPLAY_DEVICE device = { 0 };
    DEVMODE deviceMode = { 0 };

    RtlZeroMemory(&device, sizeof(DISPLAY_DEVICE));
    RtlZeroMemory(&deviceMode, sizeof(DEVMODE));

    device.cb = sizeof(DISPLAY_DEVICE);

    if (!EnumDisplayDevices(NULL, 1, &device, 0))
    {
        return GetLastError();
    }

    if (!EnumDisplaySettings(device.DeviceName, -1, &deviceMode))
    {
        return GetLastError();
    }

    deviceMode.dmPosition.x = 1350;
    deviceMode.dmPosition.y = 0;

    if (ChangeDisplaySettingsEx(
        device.DeviceName,
        &deviceMode,
        NULL,
        CDS_UPDATEREGISTRY | CDS_NORESET,
        NULL
    ) != DISP_CHANGE_SUCCESSFUL)
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI CommitDisplaySettings()
{
    if (ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL) != DISP_CHANGE_SUCCESSFUL)
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI SetCorrectDisplayConfiguration()
{
	DWORD error = ERROR_SUCCESS;
	
    if ((error = SetExtendedDisplayConfiguration()) != ERROR_SUCCESS)
    {
		return error;
    }

    if ((error = SetLeftPanelPosition()) != ERROR_SUCCESS)
    {
        return error;
    }

    if ((error = SetRightPanelPosition()) != ERROR_SUCCESS)
    {
        return error;
    }

    if ((error = CommitDisplaySettings()) != ERROR_SUCCESS)
    {
        return error;
    }

    return ERROR_SUCCESS;
}

BOOL WINAPI AreDisplaysAlreadyConfigured()
{
    DWORD Err = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    BOOL ErrorFlag = FALSE;
    HDEVINFO DevInfoList = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DevInfoData;
    WCHAR ParentDeviceInstanceId[MAX_DEVICE_ID_LEN];
    DWORD RequiredSize;
    DEVPROPTYPE DevPropType;
    SP_DEVINFO_DATA ParentDevInfoData;
    HKEY DevRegKey = NULL;
    DWORD DisplaysAlreadyConfigured = 0;
    DWORD ValueType;
    DWORD ValueSize = sizeof(DWORD);
    HKEY DevOsrRegKey = NULL;

    //
    // Create a device info list to store queried device handles.
    //
    DevInfoList = SetupDiCreateDeviceInfoList(NULL, NULL);

    if (DevInfoList == INVALID_HANDLE_VALUE)
    {
        Err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    // Add the Software Component device that called AddSoftware to the
    // device info list, which will be used to find the primary device that
    // called AddComponent in the first place.
    //
    if (!SetupDiOpenDeviceInfo(DevInfoList,
        NULL,
        NULL,
        0,
        &DevInfoData))
    {
        Err = GetLastError();
        goto cleanup;
    }

    //
    // Get the device instance id of the opened device's parent.
    //
    if (!SetupDiGetDeviceProperty(DevInfoList,
        &DevInfoData,
        &DEVPKEY_Device_Parent,
        &DevPropType,
        (PBYTE)ParentDeviceInstanceId,
        sizeof(ParentDeviceInstanceId),
        &RequiredSize,
        0))
    {
        Err = GetLastError();
        goto cleanup;
    }

    if ((DevPropType != DEVPROP_TYPE_STRING) ||
        (RequiredSize < sizeof(WCHAR)))
    {
        Err = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ParentDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    // Get the parent of the device we retrieved first, which is the device
    // that called AddComponent in its INF.
    //
    if (!SetupDiOpenDeviceInfoW(DevInfoList,
        ParentDeviceInstanceId,
        NULL,
        0,
        &ParentDevInfoData))
    {
        Err = GetLastError();
        goto cleanup;
    }

    //
    // Open up the HW registry keys of the primary device.
    //
    DevRegKey = SetupDiOpenDevRegKey(DevInfoList,
        &ParentDevInfoData,
        DICS_FLAG_GLOBAL,
        0,
        DIREG_DEV,
        KEY_READ);

    if (DevRegKey == INVALID_HANDLE_VALUE)
    {
        Err = GetLastError();
        DevRegKey = NULL;
        goto cleanup;
    }

    Err = RegOpenKeyEx(DevRegKey,
        L"Parameters",
        0,
        KEY_QUERY_VALUE,
        &DevOsrRegKey);

    if (Err != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Retrieve the registry values defined in the primary device's INF.
    // Note that if the extension INF was applied, the extension INF would
    // overwrite OperatingParams and create OperatingExceptions.
    //
    Err = RegQueryValueEx(DevOsrRegKey,
        L"DisplaysAlreadyConfigured",
        NULL,
        &ValueType,
        (LPBYTE)&DisplaysAlreadyConfigured,
        &ValueSize);

    if (Err != ERROR_SUCCESS)
    {
        DisplaysAlreadyConfigured = 0;
    }
    else if ((ValueType != REG_DWORD) ||
        (ValueSize < sizeof(DWORD)))
    {
        Err = ERROR_REGISTRY_CORRUPT;
        DisplaysAlreadyConfigured = 0;
    }

cleanup:

    if (DevOsrRegKey != NULL)
    {
        RegCloseKey(DevOsrRegKey);
    }

    if (DevRegKey != NULL)
    {
        RegCloseKey(DevRegKey);
    }

    if (DevInfoList != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(DevInfoList);
    }

    return DisplaysAlreadyConfigured == 0;
}

BOOL WINAPI MarkDisplaysAlreadyConfigured()
{
    DWORD Err = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    BOOL ErrorFlag = FALSE;
    HDEVINFO DevInfoList = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DevInfoData;
    WCHAR ParentDeviceInstanceId[MAX_DEVICE_ID_LEN];
    DWORD RequiredSize;
    DEVPROPTYPE DevPropType;
    SP_DEVINFO_DATA ParentDevInfoData;
    HKEY DevRegKey = NULL;
    DWORD DisplaysAlreadyConfigured = 1;
    DWORD ValueSize = sizeof(DWORD);
    HKEY DevOsrRegKey = NULL;

    //
    // Create a device info list to store queried device handles.
    //
    DevInfoList = SetupDiCreateDeviceInfoList(NULL, NULL);

    if (DevInfoList == INVALID_HANDLE_VALUE)
    {
        Err = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    // Add the Software Component device that called AddSoftware to the
    // device info list, which will be used to find the primary device that
    // called AddComponent in the first place.
    //
    if (!SetupDiOpenDeviceInfo(DevInfoList,
        NULL,
        NULL,
        0,
        &DevInfoData))
    {
        Err = GetLastError();
        goto cleanup;
    }

    //
    // Get the device instance id of the opened device's parent.
    //
    if (!SetupDiGetDeviceProperty(DevInfoList,
        &DevInfoData,
        &DEVPKEY_Device_Parent,
        &DevPropType,
        (PBYTE)ParentDeviceInstanceId,
        sizeof(ParentDeviceInstanceId),
        &RequiredSize,
        0))
    {
        Err = GetLastError();
        goto cleanup;
    }

    if ((DevPropType != DEVPROP_TYPE_STRING) ||
        (RequiredSize < sizeof(WCHAR)))
    {
        Err = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    ParentDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //
    // Get the parent of the device we retrieved first, which is the device
    // that called AddComponent in its INF.
    //
    if (!SetupDiOpenDeviceInfoW(DevInfoList,
        ParentDeviceInstanceId,
        NULL,
        0,
        &ParentDevInfoData))
    {
        Err = GetLastError();
        goto cleanup;
    }

    //
    // Open up the HW registry keys of the primary device.
    //
    DevRegKey = SetupDiOpenDevRegKey(DevInfoList,
        &ParentDevInfoData,
        DICS_FLAG_GLOBAL,
        0,
        DIREG_DEV,
        KEY_READ);

    if (DevRegKey == INVALID_HANDLE_VALUE)
    {
        Err = GetLastError();
        DevRegKey = NULL;
        goto cleanup;
    }

    Err = RegOpenKeyEx(DevRegKey,
        L"Parameters",
        0,
        KEY_QUERY_VALUE,
        &DevOsrRegKey);

    if (Err != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Retrieve the registry values defined in the primary device's INF.
    // Note that if the extension INF was applied, the extension INF would
    // overwrite OperatingParams and create OperatingExceptions.
    //
    Err = RegSetValueEx(DevOsrRegKey,
        L"DisplaysAlreadyConfigured",
        NULL,
        REG_DWORD,
        (LPBYTE)&DisplaysAlreadyConfigured,
        sizeof(DisplaysAlreadyConfigured));

cleanup:

    if (DevOsrRegKey != NULL)
    {
        RegCloseKey(DevOsrRegKey);
    }

    if (DevRegKey != NULL)
    {
        RegCloseKey(DevRegKey);
    }

    if (DevInfoList != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(DevInfoList);
    }

    return Err == ERROR_SUCCESS;
}