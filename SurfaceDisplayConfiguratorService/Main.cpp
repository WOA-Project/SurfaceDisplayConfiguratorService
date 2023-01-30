#include "pch.h"
#include "Main.h"
#include "DisplayConfigurator.h"
#include "AutoRotate.h"

using namespace winrt;

int main(int argc, TCHAR* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};


	StartServiceCtrlDispatcher(ServiceTable);

	return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	DWORD oobeInProgress = 0;

	HKEY key;
	DWORD type = REG_DWORD, size = 8;

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
	 * Perform tasks neccesary to start the service here
	 */

	 // Create stop event to wait on later.
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);

		goto EXIT;
	}

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	SetCorrectDisplayConfiguration();
	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", NULL, KEY_WRITE, &key) == ERROR_SUCCESS)
	{
		RegQueryValueEx(key, L"OOBEInProgress", NULL, &type, (LPBYTE)&oobeInProgress, &size);
	}

	if (oobeInProgress == 0)
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

VOID ReportSvcStatus(DWORD dwCurrentState,
					 DWORD dwWin32ExitCode,
					 DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	g_ServiceStatus.dwCurrentState = dwCurrentState;
	g_ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	g_ServiceStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		g_ServiceStatus.dwControlsAccepted = 0;
	else
		g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		g_ServiceStatus.dwCheckPoint = 0;
	else
		g_ServiceStatus.dwCheckPoint = dwCheckPoint++;

	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

DWORD WINAPI ServiceCtrlHandlerEx(
	DWORD    dwControl,
	DWORD    dwEventType,
	LPVOID   lpEventData,
	LPVOID   lpContext
)
{
	PPOWERBROADCAST_SETTING broadCastSetting = NULL;

	UNREFERENCED_PARAMETER(dwEventType);

	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		 * Perform tasks neccesary to stop the service here
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