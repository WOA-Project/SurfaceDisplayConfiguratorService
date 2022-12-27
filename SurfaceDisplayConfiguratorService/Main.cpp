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

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		// The application was ran by the user
		// So we run our main procedure

		return ServiceWorkerThread(NULL);
	}

	return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	HANDLE hThread;

	g_StatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, ServiceCtrlHandlerEx, NULL);

	if (g_StatusHandle == NULL)
	{
		goto EXIT;
	}

	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_USER_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

	/*
	 * Perform tasks neccesary to start the service here
	 */

	 // Create stop event to wait on later.
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		goto EXIT;
	}

	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

	// Start the thread that will perform the main task of the service
	hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	// Wait until our worker thread exits effectively signaling that the service needs to stop
	WaitForSingleObject(hThread, INFINITE);

	/*
	 * Perform any cleanup tasks
	 */
	CloseHandle(g_ServiceStopEvent);

	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

EXIT:
	return;
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

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);

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

VOID ExecuteAutoRotateRoutines()
{
	AutoRotateMain(g_StatusHandle);
}

VOID InitializeDisplayLayout()
{
	//if (!AreDisplaysAlreadyConfigured())
	{
		SetCorrectDisplayConfiguration();
		EnableTabletPosture();
		EnableTabletPostureTaskbar();

		//MarkDisplaysAlreadyConfigured();
	}
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	init_apartment();

	InitializeDisplayLayout();

	std::thread tAutoRotate(ExecuteAutoRotateRoutines);

	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		Sleep(1000);
	}

	uninit_apartment();

	return ERROR_SUCCESS;
}