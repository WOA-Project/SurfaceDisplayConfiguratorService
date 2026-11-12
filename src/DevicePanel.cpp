#include "pch.h"

#include <SetupAPI.h>
#include <Devpkey.h>

#include <vector>

#include "DevicePanel.h"

VOID WINAPI
DumpProperty(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, PCWSTR Name, const DEVPROPKEY *Key)
{
    BYTE Buffer[512] = {};
    DEVPROPTYPE Type = 0;
    DWORD Size = 0;

    if (!SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, Key, &Type, Buffer, sizeof(Buffer), &Size, 0))
    {
        wprintf(L"%s : <not present>\n", Name);
        return;
    }

    wprintf(L"%s\n", Name);
    wprintf(L"    Type=%08X Size=%u\n", Type, Size);

    if (Type == DEVPROP_TYPE_STRING)
    {
        wprintf(L"    Value=%s\n", (PCWSTR)Buffer);
    }
    else if (Type == DEVPROP_TYPE_UINT32)
    {
        wprintf(L"    Value=%u (0x%08X)\n", *(ULONG *)Buffer, *(ULONG *)Buffer);
    }
    else
    {
        wprintf(L"    Raw=");

        for (DWORD i = 0; i < Size; ++i)
        {
            wprintf(L"%02X ", Buffer[i]);
        }

        wprintf(L"\n");
    }
}

VOID WINAPI
DumpPanelProperties(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, PCWSTR DeviceName)
{
    wprintf(L"\n");
    wprintf(L"=====================================================\n");
    wprintf(L"%s\n", DeviceName);
    wprintf(L"=====================================================\n");

    DumpProperty(DeviceInfoSet, DeviceInfoData, L"DEVPKEY_Device_PanelId", &DEVPKEY_Device_PanelId);

    DumpProperty(DeviceInfoSet, DeviceInfoData, L"DEVPKEY_Device_PanelGroup", &DEVPKEY_Device_PanelGroup);

    DumpProperty(DeviceInfoSet, DeviceInfoData, L"DEVPKEY_Device_PanelSide", &DEVPKEY_Device_PanelSide);

    DumpProperty(
        DeviceInfoSet,
        DeviceInfoData,
        L"DEVPKEY_Device_PhysicalDeviceLocation",
        &DEVPKEY_Device_PhysicalDeviceLocation);

    DumpProperty(
        DeviceInfoSet,
        DeviceInfoData,
        L"DEVPKEY_Device_PhysicalDeviceLocationPanel",
        &DEVPKEY_Device_PhysicalDeviceLocationPanel);

    DumpProperty(
        DeviceInfoSet,
        DeviceInfoData,
        L"DEVPKEY_Device_PhysicalDeviceLocationJoint",
        &DEVPKEY_Device_PhysicalDeviceLocationJoint);

    DumpProperty(
        DeviceInfoSet,
        DeviceInfoData,
        L"DEVPKEY_Device_PhysicalDeviceLocationSpatial",
        &DEVPKEY_Device_PhysicalDeviceLocationSpatial);

    wprintf(L"\n");
}

HRESULT WINAPI
GetDisplayDevicePLD(CONST WCHAR *DevicePanelId, PACPI_PLD_V2_BUFFER Pld)
{
    HDEVINFO devInfo;
    SP_DEVINFO_DATA devData{};
    DWORD i = 0;

    if (!Pld)
    {
        return E_INVALIDARG;
    }

    ZeroMemory(Pld, sizeof(*Pld));

    devInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES);

    if (devInfo == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    devData.cbSize = sizeof(devData);

    while (SetupDiEnumDeviceInfo(devInfo, i++, &devData))
    {
        DEVPROPTYPE propType;
        DWORD bufferSize;

        SetupDiGetDevicePropertyW(devInfo, &devData, &DEVPKEY_Device_PanelId, &propType, NULL, 0, &bufferSize, 0);

        std::vector<BYTE> panelBuffer(bufferSize);

        if (!SetupDiGetDevicePropertyW(
                devInfo, &devData, &DEVPKEY_Device_PanelId, &propType, panelBuffer.data(), bufferSize, NULL, 0))
        {
            continue;
        }

        if (_wcsicmp(reinterpret_cast<PCWSTR>(panelBuffer.data()), DevicePanelId) != 0)
        {
            continue;
        }

        wprintf(L"\n");
        wprintf(L"============================================================\n");
        wprintf(L"MATCHED PANEL: %s\n", DevicePanelId);
        wprintf(L"============================================================\n");

        DumpPanelProperties(devInfo, &devData, DevicePanelId);

        DWORD requiredSize = 0;

        if (!SetupDiGetDevicePropertyW(
                devInfo,
                &devData,
                &DEVPKEY_Device_PhysicalDeviceLocation,
                &propType,
                reinterpret_cast<PBYTE>(Pld),
                sizeof(*Pld),
                &requiredSize,
                0))
        {
            continue;
        }

        if (propType != DEVPROP_TYPE_BINARY)
        {
            continue;
        }

        SetupDiDestroyDeviceInfoList(devInfo);

        return ERROR_SUCCESS;
    }

    SetupDiDestroyDeviceInfoList(devInfo);

    return ERROR_NOT_FOUND;
}