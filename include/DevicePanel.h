#pragma once

#include <windows.h>
#include <SetupAPI.h>
#include <Devpkey.h>
#include <devpropdef.h>

#include "DeviceProperties.h"

//
// DevicePanel properties discovered via reverse engineering
//
DEFINE_DEVPROPKEY(
    DEVPKEY_Device_PanelId,
    0x8DBC9C86,
    0x97A9,
    0x4BFF,
    0x9B,
    0xC6,
    0xBF,
    0xE9,
    0x5D,
    0x3E,
    0x6D,
    0xAD,
    2);

DEFINE_DEVPROPKEY(
    DEVPKEY_Device_PanelGroup,
    0x8DBC9C86,
    0x97A9,
    0x4BFF,
    0x9B,
    0xC6,
    0xBF,
    0xE9,
    0x5D,
    0x3E,
    0x6D,
    0xAD,
    3);

DEFINE_DEVPROPKEY(
    DEVPKEY_Device_PanelSide,
    0x8DBC9C86,
    0x97A9,
    0x4BFF,
    0x9B,
    0xC6,
    0xBF,
    0xE9,
    0x5D,
    0x3E,
    0x6D,
    0xAD,
    4);

//
// Additional enclosure-related properties
//
DEFINE_DEVPROPKEY(
    DEVPKEY_Device_PhysicalDeviceLocationPanel,
    0x24DAEB80,
    0x9823,
    0x4FF8,
    0x92,
    0xA6,
    0x3A,
    0x4C,
    0x61,
    0x89,
    0xF8,
    0x2C,
    2);

DEFINE_DEVPROPKEY(
    DEVPKEY_Device_PhysicalDeviceLocationJoint,
    0x52ACC9AE,
    0x7867,
    0x445E,
    0xA9,
    0x15,
    0x5C,
    0x68,
    0xAE,
    0xEA,
    0xC8,
    0x63,
    2);

DEFINE_DEVPROPKEY(
    DEVPKEY_Device_PhysicalDeviceLocationSpatial,
    0xE585291F,
    0x53AF,
    0x48AF,
    0x72,
    0x52,
    0x24,
    0x5A,
    0x68,
    0xC7,
    0x41,
    0x02,
    2);

VOID WINAPI
DumpProperty(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, PCWSTR Name, const DEVPROPKEY *Key);

VOID WINAPI
DumpPanelProperties(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, PCWSTR DeviceName);

HRESULT WINAPI
GetDisplayDevicePLD(CONST WCHAR *DevicePanelId, PACPI_PLD_V2_BUFFER Pld);