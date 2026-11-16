#include "pch.h"

#include "SensorCallbacks.h"

#include "AutoRotateService.h"

#include "PostureProcessor.h"
#include "TabletPostureManager.h"

VOID
OnPostureChanged(
    TwoPanelHingedDevicePosture const & /*Sender*/,
    TwoPanelHingedDevicePostureReadingChangedEventArgs const &Args)
{
    printf("OnPostureChanged Entry\n");

    SetWallpaperSpanStyle();

    ProcessCurrentPosture(Args.Reading());
}

VOID
OnFlipSensorReadingChanged(FlipSensor const & /*Sender*/, FlipSensorReadingChangedEventArgs const &Args)
{
    printf("OnFlipSensorReadingChanged Entry\n");

    AutoRotateService *Service = GetAutoRotateService();

    if (!Service)
    {
        return;
    }

    if (!Service->GetSensorManager().IsInitialized())
    {
        return;
    }

    SetWallpaperSpanStyle();

    FlipSensorReading Reading = Args.Reading();

    if (Reading.GestureState() == GestureState::Started)
    {
        TogglePreferredSingleScreen(Service->GetSensorManager().GetCurrentPosture());
    }
}