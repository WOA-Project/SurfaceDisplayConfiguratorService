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
#include "DisplayRotationManager.h"
#include "AutoRotate.h"
#include <powrprof.h>

//
// The system registry key for auto rotation
//
HKEY autoRotationKey = NULL;

//
// Are we already registered with the sensor?
//
BOOL AlreadySetup = FALSE;

//
// The handle the power notify event registration
//
HPOWERNOTIFY m_systemSuspendHandle = NULL;
HPOWERNOTIFY m_hScreenStateNotify = NULL;

//
// Get the default simple orientation sensor on the system
//
TwoPanelHingedDevicePosture postureSensor = TwoPanelHingedDevicePosture(nullptr);
FlipSensor flipSensor = FlipSensor(nullptr);

//
// The event token for the orientation sensor on orientation changed event
//
event_token postureEventToken;
event_token flipEventToken;

//
// Is the event subscribed
//
BOOL postureSubscribed = FALSE;
BOOL flipSubscribed = FALSE;

VOID
OnPostureChanged(
    TwoPanelHingedDevicePosture const & /*sender*/,
    TwoPanelHingedDevicePostureReadingChangedEventArgs const &args)
{
    TwoPanelHingedDevicePostureReading reading = args.Reading();
    SetPanelsOrientationState(args.Reading());
}

VOID
OnFlipSensorReadingChanged(FlipSensor const & /*sender*/, FlipSensorReadingChangedEventArgs const &args)
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
    DWORD type = REG_DWORD, size = sizeof(DWORD);

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

HRESULT
AutoRotateMain(SERVICE_STATUS_HANDLE g_StatusHandle, HANDLE g_ServiceStopEvent)
{
    try
    {
        postureSensor = TwoPanelHingedDevicePosture::GetDefaultAsync().get();
    }
    catch (...)
    {
        //
        // If no sensor is found return 1
        //
        return 1;
    }

    try
    {
        flipSensor = FlipSensor::GetDefaultAsync().get();
    }
    catch (...)
    {
        //
        // If no sensor is found return 1
        //
        return 1;
    }

    // Set initial state
    SetPanelsOrientationState(postureSensor.GetCurrentPostureAsync().get());

    //
    // Set sensor present for windows to show the auto rotation toggle in action center
    //
    if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINDOWS_AUTO_ROTATION_KEY_PATH, NULL, KEY_WRITE, &autoRotationKey)))
    {
        DWORD Enabled = 1;
        RegSetValueEx(autoRotationKey, L"SensorPresent", NULL, REG_DWORD, (LPBYTE)&Enabled, sizeof(DWORD));
        RegCloseKey(autoRotationKey);
    }

    if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINDOWS_AUTO_ROTATION_KEY_PATH, NULL, KEY_READ, &autoRotationKey)))
    {
        HANDLE hEvent = CreateEvent(NULL, true, false, NULL);
        HANDLE hEvents[2] = {hEvent, g_ServiceStopEvent};

        while (true)
        {
            SetupAutoRotation(g_StatusHandle);

            RegNotifyChangeKeyValue(autoRotationKey, false, REG_NOTIFY_CHANGE_LAST_SET, hEvent, true);

            DWORD waitResult = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

            if (waitResult == WAIT_FAILED || waitResult == (WAIT_OBJECT_0 + 1))
            {
                break;
            }
        }

        RegCloseKey(autoRotationKey);
    }
    else
    {
        RegisterEverything(g_StatusHandle);

        // Wait indefinetly
        while (true)
        {
            // Check whether to stop the service.
            if (WaitForSingleObject(g_ServiceStopEvent, INFINITE) == WAIT_FAILED)
            {
                break;
            }
        }
    }

    UnregisterEverything();

    return 0;
}
