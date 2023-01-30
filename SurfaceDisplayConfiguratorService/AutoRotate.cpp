#include "pch.h"
#include "AutoRotate.h"
#include <powrprof.h>

#define WINDOWS_AUTO_ROTATION_KEY_PATH L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AutoRotation"

CRITICAL_SECTION g_AutoRotationCriticalSection;

//
// The system registry key for auto rotation
//
HKEY autoRotationKey = NULL;

//
// Are we already registered with the sensor?
//
BOOL AlreadySetup = FALSE;

//
// Is the event subscribed
//
BOOL Subscribed = FALSE;

//
// The event token for the orientation sensor on orientation changed event
//
event_token eventToken;

//
// The handle the power notify event registration
//
HPOWERNOTIFY m_systemSuspendHandle = NULL;
HPOWERNOTIFY m_hScreenStateNotify = NULL;

//
// Get the default simple orientation sensor on the system
//
TwoPanelHingedDevicePosturePreview sensor = TwoPanelHingedDevicePosturePreview::GetDefaultAsync().get();

DWORD WINAPI SetPanelOrientation(DWORD PanelId, INT Orientation)
{
	DISPLAY_DEVICE device = {0};
	DEVMODE deviceMode = {0};

	RtlZeroMemory(&device, sizeof(DISPLAY_DEVICE));
	RtlZeroMemory(&deviceMode, sizeof(DEVMODE));

	device.cb = sizeof(DISPLAY_DEVICE);

	if (!EnumDisplayDevices(NULL, PanelId, &device, 0))
	{
		return GetLastError();
	}

	if (!EnumDisplaySettings(device.DeviceName, ENUM_CURRENT_SETTINGS, &deviceMode))
	{
		return GetLastError();
	}

	//
	// In order to switch from portrait to landscape and vice versa we need to swap the resolution width and height
	// So we check for that
	//
	if ((deviceMode.dmDisplayOrientation + Orientation) % 2 == 1)
	{
		int temp = deviceMode.dmPelsHeight;
		deviceMode.dmPelsHeight = deviceMode.dmPelsWidth;
		deviceMode.dmPelsWidth = temp;
	}

	//
	// Change the display orientation and save to the registry the changes (1 parameter for ChangeDisplaySettings)
	//
	if (deviceMode.dmFields | DM_DISPLAYORIENTATION)
	{
		deviceMode.dmDisplayOrientation = Orientation;

		if (ChangeDisplaySettingsEx(
				device.DeviceName,
				&deviceMode,
				NULL,
				CDS_UPDATEREGISTRY | CDS_GLOBAL,
				NULL) != DISP_CHANGE_SUCCESSFUL)
		{
			return GetLastError();
		}
	}

	return ERROR_SUCCESS;
}

INT WINAPI ConvertSimpleOrientationToDMDO(SimpleOrientation orientation)
{
	switch (orientation)
	{
	// Portrait
	case SimpleOrientation::NotRotated:
	{
		return DMDO_270;
	}
	// Portrait (flipped)
	case SimpleOrientation::Rotated180DegreesCounterclockwise:
	{
		return DMDO_90;
	}
	// Landscape
	case SimpleOrientation::Rotated90DegreesCounterclockwise:
	{
		return DMDO_180;
	}
	// Landscape (flipped)
	case SimpleOrientation::Rotated270DegreesCounterclockwise:
	{
		return DMDO_DEFAULT;
	}
	}

	return DMDO_DEFAULT;
}

VOID OnPostureChanged(TwoPanelHingedDevicePosturePreview const & /*sender*/, TwoPanelHingedDevicePosturePreviewReadingChangedEventArgs const &args)
{
	TwoPanelHingedDevicePosturePreviewReading reading = args.Reading();

	INT Panel1Orientation = ConvertSimpleOrientationToDMDO(reading.Panel1Orientation());
	INT Panel2Orientation = ConvertSimpleOrientationToDMDO(reading.Panel2Orientation());

	SetPanelOrientation(1, Panel1Orientation);
	SetPanelOrientation(0, Panel2Orientation);
}

VOID OnPowerEvent(
	_In_ GUID SettingGuid,
	_In_ PVOID Value,
	_In_ ULONG ValueLength,
	_Inout_opt_ PVOID Context)
{
	UNREFERENCED_PARAMETER(Context);

	if (IsEqualGUID(GUID_CONSOLE_DISPLAY_STATE, SettingGuid))
	{
		if (ValueLength != sizeof(DWORD))
		{
			return;
		}

		DWORD DisplayState = *(DWORD *)Value;

		switch (DisplayState)
		{
		case 0:
			// Display Off

			//
			// Unsubscribe to sensor events
			//
			if (Subscribed)
			{
				sensor.PostureChanged(eventToken);
				Subscribed = FALSE;
			}
			break;
		case 1:
			// Display On

			//
			// Subscribe to sensor events
			//
			if (!Subscribed)
			{
				eventToken = sensor.PostureChanged(OnPostureChanged);
				Subscribed = TRUE;
			}
			break;
		case 2:
			// Display Dimmed

			break;
		default:
			// Unknown
			break;
		}
	}
}

VOID OnSystemSuspendStatusChanged(ULONG PowerEvent)
{
	if (PowerEvent == PBT_APMSUSPEND)
	{
		// Entering

		//
		// Unsubscribe to sensor events
		//
		if (Subscribed)
		{
			sensor.PostureChanged(eventToken);
			Subscribed = FALSE;
		}
	}
	else if (PowerEvent == PBT_APMRESUMEAUTOMATIC || PowerEvent == PBT_APMRESUMESUSPEND)
	{
		// AutoResume
		// ManualResume

		//
		// Subscribe to sensor events
		//
		if (!Subscribed)
		{
			eventToken = sensor.PostureChanged(OnPostureChanged);
			Subscribed = TRUE;
		}
	}
}

// Using PVOID as per the actual typedef
ULONG CALLBACK SuspendResumeCallback(PVOID context, ULONG powerEvent, PVOID setting)
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(setting);
	OnSystemSuspendStatusChanged(powerEvent);
	return ERROR_SUCCESS;
}

VOID UnregisterEverything()
{
	if (m_systemSuspendHandle != NULL)
	{
		PowerUnregisterSuspendResumeNotification(m_systemSuspendHandle);
		m_systemSuspendHandle = NULL;
	}

	if (m_hScreenStateNotify != NULL)
	{
		UnregisterPowerSettingNotification(m_hScreenStateNotify);
		m_hScreenStateNotify = NULL;
	}

	//
	// Unsubscribe to sensor events
	//
	if (Subscribed)
	{
		sensor.PostureChanged(eventToken);
		Subscribed = FALSE;
	}

	AlreadySetup = FALSE;
}

VOID RegisterEverything(SERVICE_STATUS_HANDLE g_StatusHandle)
{
	DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS powerParams{};
	powerParams.Callback = SuspendResumeCallback;

	PowerRegisterSuspendResumeNotification(
		DEVICE_NOTIFY_CALLBACK,
		&powerParams,
		&m_systemSuspendHandle);

	if (g_StatusHandle != NULL)
	{
		m_hScreenStateNotify = RegisterPowerSettingNotification(
			g_StatusHandle,
			&GUID_CONSOLE_DISPLAY_STATE,
			DEVICE_NOTIFY_SERVICE_HANDLE);
	}

	//
	// Subscribe to sensor events
	//
	if (!Subscribed)
	{
		eventToken = sensor.PostureChanged(OnPostureChanged);
		Subscribed = TRUE;
	}

	AlreadySetup = TRUE;
}

VOID SetupAutoRotation(SERVICE_STATUS_HANDLE g_StatusHandle)
{
	DWORD type = REG_DWORD, size = 8;

	//
	// Check if rotation is enabled
	//
	DWORD enabled = 0;
	RegQueryValueEx(autoRotationKey, L"Enable", NULL, &type, (LPBYTE)&enabled, &size);

	if (enabled == 1 && AlreadySetup == FALSE)
	{
		RegisterEverything(g_StatusHandle);
	}
	else if (enabled == 0 && AlreadySetup == TRUE)
	{
		UnregisterEverything();
	}
}

int AutoRotateMain(SERVICE_STATUS_HANDLE g_StatusHandle, HANDLE g_ServiceStopEvent)
{
	//
	// If no sensor is found return 1
	//
	if (sensor == NULL)
	{
		return 1;
	}

	//
	// Set sensor present for windows to show the auto rotation toggle in action center
	//
	HKEY key;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AutoRotation", NULL, KEY_WRITE, &key) == ERROR_SUCCESS)
	{
		RegSetValueEx(key, L"SensorPresent", NULL, REG_DWORD, (LPBYTE)1, 8);
	}

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINDOWS_AUTO_ROTATION_KEY_PATH, NULL, KEY_READ, &autoRotationKey) == ERROR_SUCCESS)
	{
		SetupAutoRotation(g_StatusHandle);

		HANDLE hEvent = CreateEvent(NULL, true, false, NULL);

		RegNotifyChangeKeyValue(autoRotationKey, true, REG_NOTIFY_CHANGE_LAST_SET, hEvent, true);

		while (true)
		{
			if (WaitForSingleObject(hEvent, INFINITE) == WAIT_FAILED)
			{
				break;
			}

			SetupAutoRotation(g_StatusHandle);

			RegNotifyChangeKeyValue(autoRotationKey, false, REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES | REG_NOTIFY_CHANGE_SECURITY, hEvent, true);
		}
	}
	else
	{
		RegisterEverything(g_StatusHandle);

		// Wait indefinetly
		while (true)
		{
			// Check whether to stop the service.

			WaitForSingleObject(g_ServiceStopEvent, INFINITE);
		}
	}

	UnregisterEverything();

	return 0;
}
