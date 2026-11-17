#pragma once

#include "pch.h"

BOOL WINAPI
SubscribeSensorEvents(TwoPanelHingedDevicePosture PostureSensor, FlipSensor FlipSensorInstance);

VOID WINAPI
UnsubscribeSensorEvents(TwoPanelHingedDevicePosture PostureSensor, FlipSensor FlipSensorInstance);

BOOLEAN WINAPI
AreSensorEventsSubscribed();

extern event_token postureEventToken;
extern event_token flipEventToken;