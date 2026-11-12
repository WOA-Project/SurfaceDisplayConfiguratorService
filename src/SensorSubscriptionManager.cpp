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
    if (!g_PostureSubscribed)
    {
        postureEventToken = PostureSensor.PostureChanged(OnPostureChanged);

        g_PostureSubscribed = TRUE;
    }

    if (!g_FlipSubscribed)
    {
        flipEventToken = FlipSensorInstance.ReadingChanged(OnFlipSensorReadingChanged);

        g_FlipSubscribed = TRUE;
    }

    return TRUE;
}

VOID WINAPI
UnsubscribeSensorEvents(TwoPanelHingedDevicePosture PostureSensor, FlipSensor FlipSensorInstance)
{
    if (g_PostureSubscribed)
    {
        PostureSensor.PostureChanged(postureEventToken);

        g_PostureSubscribed = FALSE;
    }

    if (g_FlipSubscribed)
    {
        FlipSensorInstance.ReadingChanged(flipEventToken);

        g_FlipSubscribed = FALSE;
    }
}