#include "pch.h"

#include "SensorManager.h"

SensorManager::SensorManager() : m_Initialized(FALSE), m_PostureSensor(nullptr), m_FlipSensor(nullptr) {}

SensorManager::~SensorManager()
{
    Shutdown();
}

BOOLEAN
SensorManager::IsInitialized() const
{
    return m_Initialized;
}

HRESULT
SensorManager::Initialize()
{
    try
    {
        m_PostureSensor = TwoPanelHingedDevicePosture::GetDefaultAsync().get();
    }
    catch (...)
    {
        printf("Failed to obtain the posture sensor\n");
        return E_FAIL;
    }

    if (m_PostureSensor == nullptr)
    {
        return E_FAIL;
    }

    try
    {
        m_FlipSensor = FlipSensor::GetDefaultAsync().get();
    }
    catch (...)
    {
        printf("Failed to obtain the flip sensor\n");
        return E_FAIL;
    }

    if (m_FlipSensor == nullptr)
    {
        return E_FAIL;
    }

    m_Initialized = TRUE;

    return ERROR_SUCCESS;
}

VOID
SensorManager::Shutdown()
{
    if (!m_Initialized)
    {
        return;
    }

    m_PostureSensor = nullptr;
    m_FlipSensor = nullptr;

    m_Initialized = FALSE;
}

TwoPanelHingedDevicePosture
SensorManager::GetPostureSensor() const
{
    return m_PostureSensor;
}

FlipSensor
SensorManager::GetFlipSensor() const
{
    return m_FlipSensor;
}

TwoPanelHingedDevicePostureReading
SensorManager::GetCurrentPosture() const
{
    return m_PostureSensor.GetCurrentPostureAsync().get();
}