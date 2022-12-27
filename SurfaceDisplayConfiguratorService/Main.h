#pragma once
#include <Windows.h>
#include <tchar.h>

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
DWORD WINAPI ServiceCtrlHandlerEx(
    DWORD    dwControl,
    DWORD    dwEventType,
    LPVOID   lpEventData,
    LPVOID   lpContext
);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

TCHAR SERVICE_NAME[] = _T("SurfaceDisplayConfiguratorService");