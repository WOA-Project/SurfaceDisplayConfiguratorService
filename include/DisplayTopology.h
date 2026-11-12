#pragma once

#include <windows.h>
#include <SetupAPI.h>

HRESULT WINAPI
GetDisplayDeviceById(DWORD PanelId, PDISPLAY_DEVICE DisplayDevice, PDISPLAY_DEVICE DisplayDevice2);

BOOLEAN WINAPI
IsDeviceBoundToPanelId(CONST WCHAR *DeviceName, CONST WCHAR *DevicePanelId);

HRESULT WINAPI
GetDisplayDeviceByPanelId(CONST WCHAR *DevicePanelId, PDISPLAY_DEVICE DisplayDevice);