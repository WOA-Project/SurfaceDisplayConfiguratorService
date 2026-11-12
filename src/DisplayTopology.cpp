#include "pch.h"

#include <windows.h>
#include <SetupAPI.h>
#include <Devpkey.h>
#include <tchar.h>

#include "DevicePanel.h"
#include "DisplayTopology.h"

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

        devBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwBuffersize);

        if (!devBuffer)
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

        HeapFree(GetProcessHeap(), 0, devBuffer);

        if (!match)
        {
            continue;
        }

        SIZE_T DeviceNameOffset = length + 1;

        SetupDiGetDeviceProperty(devInfo, &devData, &DEVPKEY_Device_Driver, &devProptype, NULL, 0, &dwBuffersize, 0);

        devBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwBuffersize);

        if (!devBuffer)
        {
            continue;
        }

        if (!SetupDiGetDeviceProperty(
                devInfo, &devData, &DEVPKEY_Device_Driver, &devProptype, (PBYTE)devBuffer, dwBuffersize, NULL, 0))
        {
            HeapFree(GetProcessHeap(), 0, devBuffer);
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

        HeapFree(GetProcessHeap(), 0, devBuffer);

        if (!match)
        {
            continue;
        }

        SetupDiGetDeviceProperty(devInfo, &devData, &DEVPKEY_Device_PanelId, &devProptype, NULL, 0, &dwBuffersize, 0);

        devBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwBuffersize);

        if (!devBuffer)
        {
            continue;
        }

        if (!SetupDiGetDeviceProperty(
                devInfo, &devData, &DEVPKEY_Device_PanelId, &devProptype, (PBYTE)devBuffer, dwBuffersize, NULL, 0))
        {
            HeapFree(GetProcessHeap(), 0, devBuffer);
            continue;
        }

        if (lstrcmp(devBuffer, DevicePanelId))
        {
            HeapFree(GetProcessHeap(), 0, devBuffer);
            continue;
        }

        HeapFree(GetProcessHeap(), 0, devBuffer);

        SetupDiDestroyDeviceInfoList(devInfo);

        return TRUE;
    }

    SetupDiDestroyDeviceInfoList(devInfo);

    return FALSE;
}

HRESULT WINAPI
GetDisplayDeviceByPanelId(CONST WCHAR *DevicePanelId, PDISPLAY_DEVICE DisplayDevice)
{
    DWORD i = 0;

    DISPLAY_DEVICE DisplayDevice2 = {};

    while (SUCCEEDED(GetDisplayDeviceById(i++, DisplayDevice, &DisplayDevice2)))
    {
        if (IsDeviceBoundToPanelId(DisplayDevice2.DeviceID, DevicePanelId))
        {
            return ERROR_SUCCESS;
        }
    }

    return ERROR_NOT_FOUND;
}