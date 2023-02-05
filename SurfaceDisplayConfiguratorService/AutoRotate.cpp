#include "pch.h"
#include "DisplayRotationManager.h"
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
BOOL postureSubscribed = FALSE;
BOOL flipSubscribed = FALSE;

//
// The event token for the orientation sensor on orientation changed event
//
event_token postureEventToken;
event_token flipEventToken;

//
// The handle the power notify event registration
//
HPOWERNOTIFY m_systemSuspendHandle = NULL;
HPOWERNOTIFY m_hScreenStateNotify = NULL;

//
// Get the default simple orientation sensor on the system
//
TwoPanelHingedDevicePosture postureSensor = TwoPanelHingedDevicePosture::GetDefaultAsync().get();
FlipSensor flipSensor = FlipSensor::GetDefaultAsync().get();

VOID
OnPostureChanged(
    TwoPanelHingedDevicePosture const& /*sender*/,
    TwoPanelHingedDevicePostureReadingChangedEventArgs const& args)
{
    TwoPanelHingedDevicePostureReading reading = args.Reading();
    SetPanelsOrientationState(args.Reading());
}

VOID
OnFlipSensorReadingChanged(FlipSensor const& /*sender*/,
    FlipSensorReadingChangedEventArgs const& args)
{
    FlipSensorReading reading = args.Reading();

    if (reading.GestureState() == GestureState::Completed)
    {
        ToggleFavoriteSingleScreenDisplay();
        SetPanelsOrientationState(postureSensor.GetCurrentPostureAsync().get());
    }
}

VOID
OnPowerEvent(_In_ GUID SettingGuid, _In_ PVOID Value, _In_ ULONG ValueLength, _Inout_opt_ PVOID Context)
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
            if (postureSubscribed)
            {
                postureSensor.PostureChanged(postureEventToken);
                postureSubscribed = FALSE;
            }

            if (flipSubscribed)
            {
                flipSensor.ReadingChanged(flipEventToken);
                flipSubscribed = FALSE;
            }
            break;
        case 1:
            // Display On

            //
            // Subscribe to sensor events
            //
            if (!postureSubscribed)
            {
                postureEventToken = postureSensor.PostureChanged(OnPostureChanged);
                postureSubscribed = TRUE;
            }

            if (!flipSubscribed)
            {
                flipEventToken = flipSensor.ReadingChanged(OnFlipSensorReadingChanged);
                flipSubscribed = TRUE;
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

VOID
OnSystemSuspendStatusChanged(ULONG PowerEvent)
{
    if (PowerEvent == PBT_APMSUSPEND)
    {
        // Entering

        //
        // Unsubscribe to sensor events
        //
        if (postureSubscribed)
        {
            postureSensor.PostureChanged(postureEventToken);
            postureSubscribed = FALSE;
        }

        if (flipSubscribed)
        {
            flipSensor.ReadingChanged(flipEventToken);
            flipSubscribed = FALSE;
        }
    }
    else if (PowerEvent == PBT_APMRESUMEAUTOMATIC || PowerEvent == PBT_APMRESUMESUSPEND)
    {
        // AutoResume
        // ManualResume

        //
        // Subscribe to sensor events
        //
        if (!postureSubscribed)
        {
            postureEventToken = postureSensor.PostureChanged(OnPostureChanged);
            postureSubscribed = TRUE;
        }

        if (!flipSubscribed)
        {
            flipEventToken = flipSensor.ReadingChanged(OnFlipSensorReadingChanged);
            flipSubscribed = TRUE;
        }
    }
}

// Using PVOID as per the actual typedef
ULONG CALLBACK
SuspendResumeCallback(PVOID context, ULONG powerEvent, PVOID setting)
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(setting);
    OnSystemSuspendStatusChanged(powerEvent);
    return ERROR_SUCCESS;
}

VOID
UnregisterEverything()
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
    if (postureSubscribed)
    {
        postureSensor.PostureChanged(postureEventToken);
        postureSubscribed = FALSE;
    }

    if (flipSubscribed)
    {
        flipSensor.ReadingChanged(flipEventToken);
        flipSubscribed = FALSE;
    }

    AlreadySetup = FALSE;
}

VOID
RegisterEverything(SERVICE_STATUS_HANDLE g_StatusHandle)
{
    DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS powerParams{};
    powerParams.Callback = SuspendResumeCallback;

    PowerRegisterSuspendResumeNotification(DEVICE_NOTIFY_CALLBACK, &powerParams, &m_systemSuspendHandle);

    if (g_StatusHandle != NULL)
    {
        m_hScreenStateNotify =
            RegisterPowerSettingNotification(g_StatusHandle, &GUID_CONSOLE_DISPLAY_STATE, DEVICE_NOTIFY_SERVICE_HANDLE);
    }

    //
    // Subscribe to sensor events
    //
    if (!postureSubscribed)
    {
        postureEventToken = postureSensor.PostureChanged(OnPostureChanged);
        postureSubscribed = TRUE;
    }

    if (!flipSubscribed)
    {
        flipEventToken = flipSensor.ReadingChanged(OnFlipSensorReadingChanged);
        flipSubscribed = TRUE;
    }

    AlreadySetup = TRUE;
}

VOID
SetupAutoRotation(SERVICE_STATUS_HANDLE g_StatusHandle)
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

int
AutoRotateMain(SERVICE_STATUS_HANDLE g_StatusHandle, HANDLE g_ServiceStopEvent)
{
    postureSensor = TwoPanelHingedDevicePosture::GetDefaultAsync().get();

    //
    // If no sensor is found return 1
    //
    if (postureSensor == NULL)
    {
        return 1;
    }

    flipSensor = FlipSensor::GetDefaultAsync().get();

    //
    // If no sensor is found return 1
    //
    if (flipSensor == NULL)
    {
        return 1;
    }

    // Set initial state
    SetPanelsOrientationState(postureSensor.GetCurrentPostureAsync().get());

    //
    // Set sensor present for windows to show the auto rotation toggle in action center
    //
    HKEY key;
    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AutoRotation", NULL, KEY_WRITE, &key) ==
        ERROR_SUCCESS)
    {
        RegSetValueEx(key, L"SensorPresent", NULL, REG_DWORD, (LPBYTE)1, 8);
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINDOWS_AUTO_ROTATION_KEY_PATH, NULL, KEY_READ, &autoRotationKey) ==
        ERROR_SUCCESS)
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

            RegNotifyChangeKeyValue(
                autoRotationKey,
                false,
                REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES |
                    REG_NOTIFY_CHANGE_SECURITY,
                hEvent,
                true);
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
