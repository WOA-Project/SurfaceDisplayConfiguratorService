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
#include "Main.h"
#include "TabletPostureManager.h"
#include "DisplayRotationManager.h"
#include "AutoRotate.h"

using namespace winrt;

BOOLEAN WINAPI
IsOOBEInProgress()
{
    DWORD oobeInProgress = 0;

    HKEY key;
    DWORD type = REG_DWORD, size = 8;

    if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", NULL, KEY_WRITE, &key)))
    {
        RegQueryValueEx(key, L"OOBEInProgress", NULL, &type, (LPBYTE)&oobeInProgress, &size);
        RegCloseKey(key);
    }

    return oobeInProgress == 1;
}

int
main(int argc, TCHAR *argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    SERVICE_TABLE_ENTRY ServiceTable[] = {{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain}, {NULL, NULL}};

    StartServiceCtrlDispatcher(ServiceTable);

    return 0;
}

VOID WINAPI
ServiceMain(DWORD argc, LPTSTR *argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    g_StatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, ServiceCtrlHandlerEx, NULL);

    if (g_StatusHandle == NULL)
    {
        goto EXIT;
    }

    // Tell the service controller we are starting
    g_ServiceStatus.dwServiceType = SERVICE_USER_OWN_PROCESS;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    /*
     * Perform tasks necessary to start the service here
     */

    // Create stop event to wait on later.
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        ReportSvcStatus(SERVICE_STOPPED, HRESULT_FROM_WIN32(GetLastError()), 0);

        goto EXIT;
    }

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    InitializeDisplayRotationManager();
    SetExtendedDisplayConfiguration();
    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    if (!IsOOBEInProgress())
    {
        EnableTabletPosture();
        ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
        EnableTabletPostureTaskbar();
    }

    // Tell the service controller we are started
    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

    // Start the thread that will perform the main task of the service

    // Wait until our worker thread exits effectively signaling that the service needs to stop

    init_apartment();

    AutoRotateMain(g_StatusHandle, g_ServiceStopEvent);

    uninit_apartment();

    /*
     * Perform any cleanup tasks
     */
    CloseHandle(g_ServiceStopEvent);

    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);

EXIT:
    return;
}

VOID
ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    g_ServiceStatus.dwCurrentState = dwCurrentState;
    g_ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    g_ServiceStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        g_ServiceStatus.dwControlsAccepted = 0;
    else
        g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
        g_ServiceStatus.dwCheckPoint = 0;
    else
        g_ServiceStatus.dwCheckPoint = dwCheckPoint++;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

DWORD WINAPI
ServiceCtrlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    PPOWERBROADCAST_SETTING broadCastSetting = NULL;

    UNREFERENCED_PARAMETER(dwEventType);

    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks necessary to stop the service here
         */

        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        ReportSvcStatus(g_ServiceStatus.dwCurrentState, NO_ERROR, 0);

        break;

    case SERVICE_CONTROL_POWEREVENT:
        broadCastSetting = (PPOWERBROADCAST_SETTING)lpEventData;
        OnPowerEvent(broadCastSetting->PowerSetting, broadCastSetting->Data, broadCastSetting->DataLength, lpContext);

        break;

    default:
        break;
    }

    return ERROR_SUCCESS;
}