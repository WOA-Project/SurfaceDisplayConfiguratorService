#pragma once

#include "pch.h"

#include "SensorManager.h"

class AutoRotateService
{
public:
    AutoRotateService();
    ~AutoRotateService();

    HRESULT Initialize();
    VOID Shutdown();

    VOID Run();

    BOOLEAN IsInitialized() const;

    SensorManager &GetSensorManager();

    TwoPanelHingedDevicePostureReading GetCurrentPosture();

private:
    HRESULT InitializeSensors();
    HRESULT InitializeInfrastructure();

    HRESULT OpenAutoRotationKey(HKEY *Key);

    VOID UpdateAutoRotationState(HKEY AutoRotationKey);
    VOID MonitorAutoRotationSettings();

private:
    BOOLEAN m_Initialized;

    SensorManager m_SensorManager;

    HANDLE m_TaskbarMonitorThread;
};

AutoRotateService *WINAPI
GetAutoRotateService();