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
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "AutoRotate.h"
#include "ActiveMonitorWindowHandler.h"

TCHAR SVCNAME[] = TEXT("SurfaceDisplayConfiguratorService");

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;

DWORD WINAPI SvcCtrlHandler(DWORD, DWORD, LPVOID, LPVOID);
VOID WINAPI
SvcMain(DWORD, LPTSTR *);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID
SvcInit(DWORD, LPTSTR *);

HPOWERNOTIFY m_hScreenStateNotify = NULL;

//
// Purpose:
//   Entry point for the process
//
// Parameters:
//   None
//
// Return value:
//   None, defaults to 0 (zero)
//
int APIENTRY
_tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    // TO_DO: Add any additional services for the process to this table.
    SERVICE_TABLE_ENTRY DispatchTable[] = {{SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain}, {NULL, NULL}};

    // This call returns when the service has stopped.
    // The process should simply terminate when the call returns.

    StartServiceCtrlDispatcher(DispatchTable);
}

//
// Purpose:
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
//
// Return value:
//   None.
//
VOID WINAPI
SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    // Register the handler function for the service

    gSvcStatusHandle = RegisterServiceCtrlHandlerEx(SVCNAME, SvcCtrlHandler, NULL);

    if (!gSvcStatusHandle)
    {
        return;
    }

    m_hScreenStateNotify =
        RegisterPowerSettingNotification(gSvcStatusHandle, &GUID_CONSOLE_DISPLAY_STATE, DEVICE_NOTIFY_SERVICE_HANDLE);

    // These SERVICE_STATUS members remain as set here

    gSvcStatus.dwServiceType = SERVICE_USER_OWN_PROCESS;
    gSvcStatus.dwServiceSpecificExitCode = 0;

    // Report initial status to the SCM

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    // Perform service-specific initialization and work.

    SvcInit(dwArgc, lpszArgv);

    if (m_hScreenStateNotify != NULL)
    {
        UnregisterPowerSettingNotification(m_hScreenStateNotify);
        m_hScreenStateNotify = NULL;
    }
}

//
// Purpose:
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
//
// Return value:
//   None
//
VOID
SvcInit(DWORD, LPTSTR *)
{
    // TO_DO: Declare and set any required variables.
    //   Be sure to periodically call ReportSvcStatus() with
    //   SERVICE_START_PENDING. If initialization fails, call
    //   ReportSvcStatus with SERVICE_STOPPED.

    // Create an event. The control handler function, SvcCtrlHandler,
    // signals this event when it receives the stop control code.

    ghSvcStopEvent = CreateEvent(
        NULL,  // default security attributes
        TRUE,  // manual reset event
        FALSE, // not signaled
        NULL); // no name

    if (ghSvcStopEvent == NULL)
    {
        ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    // Report running status when initialization is complete.

    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

    // Perform work until service stops.
    std::thread t1(AutoRotateMain);
    std::thread t2(ActiveMonitorWindowHandlerMain);

    while (1)
    {
        // Check whether to stop the service.

        WaitForSingleObject(ghSvcStopEvent, INFINITE);

        t1.detach();
        t2.detach();

        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }
}

//
// Purpose:
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation,
//     in milliseconds
//
// Return value:
//   None
//
VOID
ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure.

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else
        gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
        gSvcStatus.dwCheckPoint = 0;
    else
        gSvcStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status of the service to the SCM.
    SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

//
// Purpose:
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
//
// Return value:
//   None
//
DWORD WINAPI
SvcCtrlHandler(DWORD dwCtrl, DWORD, LPVOID lpEventData, LPVOID lpContext)
{
    // Handle the requested control code.
    PPOWERBROADCAST_SETTING broadCastSetting = NULL;

    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

        // Signal the service to stop.

        SetEvent(ghSvcStopEvent);
        ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

        return ERROR_SUCCESS;

    case SERVICE_CONTROL_INTERROGATE:
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