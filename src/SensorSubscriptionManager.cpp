#include "pch.h"

#include "SensorSubscriptionManager.h"

extern VOID
OnPostureChanged(
    TwoPanelHingedDevicePosture const &Sender,
    TwoPanelHingedDevicePostureReadingChangedEventArgs const &Args);

extern VOID
OnFlipSensorReadingChanged(FlipSensor const &Sender, FlipSensorReadingChangedEventArgs const &Args);

event_token postureEventToken{};
event_token flipEventToken{};

static BOOLEAN g_PostureSubscribed = FALSE;
static BOOLEAN g_FlipSubscribed = FALSE;

BOOLEAN WINAPI
AreSensorEventsSubscribed()
{
    return g_PostureSubscribed && g_FlipSubscribed;
}

BOOL WINAPI
SubscribeSensorEvents(TwoPanelHingedDevicePosture PostureSensor, FlipSensor FlipSensorInstance)
{
    if (!PostureSensor)
    {
        OutputDebugString(L"Posture sensor unavailable\n");
        return FALSE;
    }

    if (!FlipSensorInstance)
    {
        OutputDebugString(L"Flip sensor unavailable\n");
        return FALSE;
    }

    if (!g_PostureSubscribed)
    {
        if (PostureSensor)
        {
            postureEventToken = PostureSensor.PostureChanged(OnPostureChanged);

            g_PostureSubscribed = TRUE;
        }
    }

    if (!g_FlipSubscribed)
    {
        if (FlipSensorInstance)
        {
            flipEventToken = FlipSensorInstance.ReadingChanged(OnFlipSensorReadingChanged);

            g_FlipSubscribed = TRUE;
        }
    }

    return TRUE;
}

VOID WINAPI
UnsubscribeSensorEvents(TwoPanelHingedDevicePosture PostureSensor, FlipSensor FlipSensorInstance)
{
    if (g_PostureSubscribed && PostureSensor)
    {
        PostureSensor.PostureChanged(postureEventToken);

        g_PostureSubscribed = FALSE;
    }

    if (g_FlipSubscribed && FlipSensorInstance)
    {
        FlipSensorInstance.ReadingChanged(flipEventToken);

        g_FlipSubscribed = FALSE;
    }
}