#pragma once

#include "pch.h"

class SensorManager
{
public:
    SensorManager();
    ~SensorManager();

    HRESULT Initialize();
    VOID Shutdown();

    BOOLEAN IsInitialized() const;

    TwoPanelHingedDevicePosture GetPostureSensor() const;
    FlipSensor GetFlipSensor() const;

    TwoPanelHingedDevicePostureReading GetCurrentPosture() const;

private:
    BOOLEAN m_Initialized;

    TwoPanelHingedDevicePosture m_PostureSensor;
    FlipSensor m_FlipSensor;
};