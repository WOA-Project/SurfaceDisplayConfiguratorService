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
#include "TabletPostureManager.h"
#include "AutoRotate.h"
#include <powrprof.h>
#include <tchar.h>

#define WINDOWS_AUTO_ROTATION_KEY_PATH _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AutoRotation")

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

//
// Get the default simple orientation sensor on the system
//
TwoPanelHingedDevicePosture g_PostureSensor{nullptr};
FlipSensor g_FlipSensor{nullptr};

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

BOOL FoundAllSensors = FALSE;

BOOLEAN IsDisplay1SingleScreenFavorite = FALSE;

CRITICAL_SECTION g_AutoRotationCriticalSection;

INT WINAPI
ConvertSimpleOrientationToDMDO(SimpleOrientation orientation)
{
    switch (orientation)
    {
    case SimpleOrientation::NotRotated: {
        return DMDO_DEFAULT;
    }
    case SimpleOrientation::Rotated90DegreesCounterclockwise: {
        return DMDO_270;
    }
    case SimpleOrientation::Rotated180DegreesCounterclockwise: {
        return DMDO_180;
    }
    case SimpleOrientation::Rotated270DegreesCounterclockwise: {
        return DMDO_90;
    }
    }

    return DMDO_DEFAULT;
}

HRESULT WINAPI
SetPanelsOrientationState(TwoPanelHingedDevicePostureReading reading)
{
    SimpleOrientation Panel1SimpleOrientation = reading.Panel1Orientation();
    SimpleOrientation Panel2SimpleOrientation = reading.Panel2Orientation();

    CONST WCHAR *panel1Id = reading.Panel1Id().c_str();
    CONST WCHAR *panel2Id = reading.Panel2Id().c_str();

    BOOLEAN Panel1UnknownOrientation =
        Panel1SimpleOrientation == SimpleOrientation::Faceup || Panel1SimpleOrientation == SimpleOrientation::Facedown;
    BOOLEAN Panel2UnknownOrientation =
        Panel2SimpleOrientation == SimpleOrientation::Faceup || Panel2SimpleOrientation == SimpleOrientation::Facedown;

    if (Panel1UnknownOrientation && !Panel2UnknownOrientation)
    {
        Panel1SimpleOrientation = Panel2SimpleOrientation;
    }
    else if (Panel2UnknownOrientation && !Panel1UnknownOrientation)
    {
        Panel2SimpleOrientation = Panel1SimpleOrientation;
    }

    INT Panel1Orientation = ConvertSimpleOrientationToDMDO(Panel1SimpleOrientation);
    INT Panel2Orientation = ConvertSimpleOrientationToDMDO(Panel2SimpleOrientation);

    if (reading.HingeState() == Windows::Internal::System::HingeState::Full)
    {
        return SetDisplayStates(
            panel1Id,
            panel2Id,
            Panel1Orientation,
            Panel2Orientation,
            IsDisplay1SingleScreenFavorite ? TRUE : FALSE,
            IsDisplay1SingleScreenFavorite ? FALSE : TRUE);
    }
    // All displays must be enabled
    else
    {
        return SetDisplayStates(panel1Id, panel2Id, Panel1Orientation, Panel2Orientation, TRUE, TRUE);
    }

    return ERROR_SUCCESS;
}

VOID WINAPI
ToggleFavoriteSingleScreenDisplay()
{
    EnterCriticalSection(&g_AutoRotationCriticalSection);
    IsDisplay1SingleScreenFavorite = IsDisplay1SingleScreenFavorite ? FALSE : TRUE;
    SetPanelsOrientationState(g_PostureSensor.GetCurrentPostureAsync().get());
    LeaveCriticalSection(&g_AutoRotationCriticalSection);
}

VOID WINAPI
TogglePostureScreenOrientationState()
{
    EnterCriticalSection(&g_AutoRotationCriticalSection);
    SetPanelsOrientationState(g_PostureSensor.GetCurrentPostureAsync().get());
    LeaveCriticalSection(&g_AutoRotationCriticalSection);
}

VOID
OnPostureChanged(
    TwoPanelHingedDevicePosture const & /*sender*/,
    TwoPanelHingedDevicePostureReadingChangedEventArgs const & /*args*/)
{
    TogglePostureScreenOrientationState();
}

VOID
OnFlipSensorReadingChanged(FlipSensor const & /*sender*/, FlipSensorReadingChangedEventArgs const &args)
{
    if (!FoundAllSensors)
    {
        return;
    }

    FlipSensorReading reading = args.Reading();
    if (reading.GestureState() == GestureState::Completed)
    {
        ToggleFavoriteSingleScreenDisplay();
    }
}

VOID
OnSystemSuspendStatusChanged(
    ULONG PowerEvent,
    TwoPanelHingedDevicePosture const &postureSensor,
    FlipSensor const &flipSensor)
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

VOID
OnPowerEvent(_In_ GUID SettingGuid, _In_ PVOID Value, _In_ ULONG ValueLength, _Inout_opt_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    if (!FoundAllSensors)
    {
        return;
    }

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
                g_PostureSensor.PostureChanged(postureEventToken);
                postureSubscribed = FALSE;
            }

            if (flipSubscribed)
            {
                g_FlipSensor.ReadingChanged(flipEventToken);
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
                postureEventToken = g_PostureSensor.PostureChanged(OnPostureChanged);
                postureSubscribed = TRUE;
            }

            if (!flipSubscribed)
            {
                flipEventToken = g_FlipSensor.ReadingChanged(OnFlipSensorReadingChanged);
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

// Using PVOID as per the actual typedef
ULONG CALLBACK
SuspendResumeCallback(PVOID context, ULONG powerEvent, PVOID setting)
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(setting);

    if (FoundAllSensors)
    {
        OnSystemSuspendStatusChanged(powerEvent, g_PostureSensor, g_FlipSensor);
    }

    return ERROR_SUCCESS;
}

VOID
RegisterEverything(TwoPanelHingedDevicePosture const &postureSensor, FlipSensor const &flipSensor)
{
    DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS powerParams{};
    powerParams.Callback = SuspendResumeCallback;

    PowerRegisterSuspendResumeNotification(DEVICE_NOTIFY_CALLBACK, &powerParams, &m_systemSuspendHandle);

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
UnregisterEverything(TwoPanelHingedDevicePosture const &postureSensor, FlipSensor const &flipSensor)
{
    if (m_systemSuspendHandle != NULL)
    {
        PowerUnregisterSuspendResumeNotification(m_systemSuspendHandle);
        m_systemSuspendHandle = NULL;
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
SetupAutoRotation(TwoPanelHingedDevicePosture const &postureSensor, FlipSensor const &flipSensor)
{
    DWORD type = REG_DWORD, size = sizeof(DWORD);

    //
    // Check if rotation is enabled
    //
    DWORD enabled = 0;
    RegQueryValueEx(autoRotationKey, _T("Enable"), NULL, &type, (LPBYTE)&enabled, &size);

    if (enabled == 1 && AlreadySetup == FALSE)
    {
        RegisterEverything(postureSensor, flipSensor);
    }
    else if (enabled == 0 && AlreadySetup == TRUE)
    {
        UnregisterEverything(postureSensor, flipSensor);
    }
}

BOOLEAN WINAPI
IsOOBEInProgress()
{
    DWORD oobeInProgress = 0;

    HKEY key;
    DWORD type = REG_DWORD, size = 8;

    if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\Setup"), NULL, KEY_WRITE, &key)))
    {
        RegQueryValueEx(key, _T("OOBEInProgress"), NULL, &type, (LPBYTE)&oobeInProgress, &size);
        RegCloseKey(key);
    }

    return oobeInProgress == 1;
}

VOID
AutoRotateMain()
{
    init_apartment();

    try
    {
        g_PostureSensor = TwoPanelHingedDevicePosture::GetDefaultAsync().get();
    }
    catch (...)
    {
        uninit_apartment();
        return;
    }

    if (g_PostureSensor == NULL)
    {
        uninit_apartment();
        return;
    }

    try
    {
        g_FlipSensor = FlipSensor::GetDefaultAsync().get();
    }
    catch (...)
    {
        uninit_apartment();
        return;
    }

    if (g_FlipSensor == NULL)
    {
        uninit_apartment();
        return;
    }

    FoundAllSensors = TRUE;

    InitializeCriticalSectionAndSpinCount(&g_AutoRotationCriticalSection, 0x00000400);

    if (!IsOOBEInProgress())
    {
        SetTabletPostureState(TRUE);
        SetTabletPostureTaskbarState(TRUE);
    }

    // Set initial state
    TogglePostureScreenOrientationState();

    //
    // Set sensor present for windows to show the auto rotation toggle in action center
    //
    if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINDOWS_AUTO_ROTATION_KEY_PATH, NULL, KEY_WRITE, &autoRotationKey)))
    {
        DWORD Enabled = 1;
        RegSetValueEx(autoRotationKey, _T("SensorPresent"), NULL, REG_DWORD, (LPBYTE)&Enabled, sizeof(DWORD));
        RegCloseKey(autoRotationKey);
    }

    if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINDOWS_AUTO_ROTATION_KEY_PATH, NULL, KEY_READ, &autoRotationKey)))
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        while (true)
        {
            SetupAutoRotation(g_PostureSensor, g_FlipSensor);

            RegNotifyChangeKeyValue(autoRotationKey, false, REG_NOTIFY_CHANGE_LAST_SET, hEvent, true);

            // Check whether to stop the service.
            if (WaitForSingleObject(hEvent, INFINITE) == WAIT_FAILED)
            {
                break;
            }
        }

        RegCloseKey(autoRotationKey);
    }
    else
    {
        RegisterEverything(g_PostureSensor, g_FlipSensor);

        // Wait indefinitely
        while (true)
        {
            Sleep(4294967295ull);
        }
    }

    UnregisterEverything(g_PostureSensor, g_FlipSensor);

    FoundAllSensors = FALSE;
    g_PostureSensor = NULL;
    g_FlipSensor = NULL;

    if (!IsOOBEInProgress())
    {
        SetTabletPostureTaskbarState(FALSE);
        SetTabletPostureState(FALSE);
    }

    uninit_apartment();

    return;
}
