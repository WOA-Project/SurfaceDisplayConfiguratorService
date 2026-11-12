#include "pch.h"

#include <powrprof.h>

#include "PowerManager.h"
#include "SensorSubscriptionManager.h"

HPOWERNOTIFY g_SystemSuspendHandle = NULL;

static TwoPanelHingedDevicePosture g_PostureSensorForPower{nullptr};
static FlipSensor g_FlipSensorForPower{nullptr};

static VOID
HandleSuspendResume(ULONG PowerEvent)
{
    if (PowerEvent == PBT_APMSUSPEND)
    {
        UnsubscribeSensorEvents(g_PostureSensorForPower, g_FlipSensorForPower);
    }
    else if (PowerEvent == PBT_APMRESUMEAUTOMATIC || PowerEvent == PBT_APMRESUMESUSPEND)
    {
        SubscribeSensorEvents(g_PostureSensorForPower, g_FlipSensorForPower);
    }
}

ULONG CALLBACK
SuspendResumeCallback(PVOID Context, ULONG PowerEvent, PVOID Setting)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Setting);

    HandleSuspendResume(PowerEvent);

    return ERROR_SUCCESS;
}

VOID WINAPI
HandleDisplayPowerEvent(GUID SettingGuid, PVOID Value, ULONG ValueLength)
{
    if (!IsEqualGUID(GUID_CONSOLE_DISPLAY_STATE, SettingGuid))
    {
        return;
    }

    if (ValueLength != sizeof(DWORD))
    {
        return;
    }

    DWORD DisplayState = *(DWORD *)Value;

    switch (DisplayState)
    {
    case 0:
        //
        // Display Off
        //
        UnsubscribeSensorEvents(g_PostureSensorForPower, g_FlipSensorForPower);
        break;

    case 1:
        //
        // Display On
        //
        SubscribeSensorEvents(g_PostureSensorForPower, g_FlipSensorForPower);
        break;

    case 2:
        //
        // Display Dimmed
        //
        break;

    default:
        break;
    }
}

VOID WINAPI
InitializePowerManagement(TwoPanelHingedDevicePosture PostureSensor, FlipSensor FlipSensorInstance)
{
    g_PostureSensorForPower = PostureSensor;

    g_FlipSensorForPower = FlipSensorInstance;

    DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS Params{};

    Params.Callback = SuspendResumeCallback;

    PowerRegisterSuspendResumeNotification(DEVICE_NOTIFY_CALLBACK, &Params, &g_SystemSuspendHandle);
}

VOID WINAPI
ShutdownPowerManagement()
{
    if (g_SystemSuspendHandle)
    {
        PowerUnregisterSuspendResumeNotification(g_SystemSuspendHandle);

        g_SystemSuspendHandle = NULL;
    }
}