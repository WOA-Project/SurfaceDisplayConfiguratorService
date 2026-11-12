#include "pch.h"

#include "AutoRotateService.h"

#include "AutoRotateContext.h"

#include "SensorManager.h"
#include "PowerManager.h"
#include "PostureProcessor.h"
#include "SensorSubscriptionManager.h"

#include "TabletPostureManager.h"
#include "DisplayRotationManager.h"
#include "WorkAreas.h"

#include "DisplayIdentityCache.h"
#include <tchar.h>

constexpr PCWSTR WINDOWS_AUTO_ROTATION_KEY_PATH = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AutoRotation";

static AutoRotateService *g_ServiceInstance = nullptr;

AutoRotateService *WINAPI
GetAutoRotateService()
{
    return g_ServiceInstance;
}

AutoRotateService::AutoRotateService() : m_Initialized(FALSE), m_TaskbarMonitorThread(nullptr)
{
    g_ServiceInstance = this;
}

AutoRotateService::~AutoRotateService()
{
    if (g_ServiceInstance == this)
    {
        g_ServiceInstance = nullptr;
    }

    Shutdown();
}

BOOLEAN
AutoRotateService::IsInitialized() const
{
    return m_Initialized;
}

SensorManager &
AutoRotateService::GetSensorManager()
{
    return m_SensorManager;
}

HRESULT
AutoRotateService::InitializeSensors()
{
    return m_SensorManager.Initialize();
}

HRESULT
AutoRotateService::InitializeInfrastructure()
{
    InitializeAutoRotateContext();

    HRESULT Status = g_DisplayIdentityCache.Initialize();

    if (FAILED(Status))
    {
        return Status;
    }

    InitializeTabletPostureSubscriptions();

    m_TaskbarMonitorThread = CreateThread(nullptr, 0, TaskbarRegistryMonitorThread, nullptr, 0, nullptr);

    InitializePowerManagement(m_SensorManager.GetPostureSensor(), m_SensorManager.GetFlipSensor());

    SubscribeSensorEvents(m_SensorManager.GetPostureSensor(), m_SensorManager.GetFlipSensor());

    return ERROR_SUCCESS;
}

HRESULT
AutoRotateService::Initialize()
{
    init_apartment();

    SetExtendedDisplayConfiguration();

    SetTabletPostureState(TRUE);
    SetTabletPostureTaskbarState(TRUE);

    UpdateMonitorWorkAreas();
    SetWallpaperSpanStyle();

    HRESULT Status = InitializeSensors();

    if (FAILED(Status))
    {
        uninit_apartment();
        return Status;
    }

    Status = InitializeInfrastructure();

    if (FAILED(Status))
    {
        m_SensorManager.Shutdown();
        uninit_apartment();
        return Status;
    }

    m_Initialized = TRUE;

    return ERROR_SUCCESS;
}

TwoPanelHingedDevicePostureReading
AutoRotateService::GetCurrentPosture()
{
    return m_SensorManager.GetCurrentPosture();
}

HRESULT
AutoRotateService::OpenAutoRotationKey(HKEY *Key)
{
    if (!Key)
    {
        return E_INVALIDARG;
    }

    LONG Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINDOWS_AUTO_ROTATION_KEY_PATH, 0, KEY_READ, Key);

    return HRESULT_FROM_WIN32(Result);
}

VOID
AutoRotateService::UpdateAutoRotationState(HKEY AutoRotationKey)
{
    DWORD Type = REG_DWORD;
    DWORD Size = sizeof(DWORD);

    DWORD Enabled = 0;

    RegQueryValueEx(AutoRotationKey, _T("Enable"), nullptr, &Type, reinterpret_cast<LPBYTE>(&Enabled), &Size);

    g_AutoRotateContext.AutoRotationEnabled = (Enabled != 0);
}

VOID
AutoRotateService::MonitorAutoRotationSettings()
{
    HKEY AutoRotationKey = nullptr;

    if (FAILED(OpenAutoRotationKey(&AutoRotationKey)))
    {
        Sleep(INFINITE);
        return;
    }

    HANDLE Event = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (!Event)
    {
        RegCloseKey(AutoRotationKey);
        return;
    }

    while (TRUE)
    {
        UpdateAutoRotationState(AutoRotationKey);

        RegNotifyChangeKeyValue(AutoRotationKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, Event, TRUE);

        if (WaitForSingleObject(Event, INFINITE) == WAIT_FAILED)
        {
            break;
        }

        ResetEvent(Event);
    }

    CloseHandle(Event);

    RegCloseKey(AutoRotationKey);
}

VOID
AutoRotateService::Run()
{
    ProcessCurrentPosture(GetCurrentPosture());

    MonitorAutoRotationSettings();
}

VOID
AutoRotateService::Shutdown()
{
    if (!m_Initialized)
    {
        return;
    }

    ShutdownPowerManagement();

    UnsubscribeSensorEvents(m_SensorManager.GetPostureSensor(), m_SensorManager.GetFlipSensor());

    ShutdownTabletPostureSubscriptions();

    if (m_TaskbarMonitorThread)
    {
        g_StopTaskbarMonitor = TRUE;

        WaitForSingleObject(m_TaskbarMonitorThread, 5000);

        CloseHandle(m_TaskbarMonitorThread);

        m_TaskbarMonitorThread = nullptr;
    }

    m_SensorManager.Shutdown();

    UpdateMonitorWorkAreas();

    SetTabletPostureState(FALSE);
    SetTabletPostureTaskbarState(FALSE);

    SetWallpaperSpanStyle();

    DestroyAutoRotateContext();

    uninit_apartment();

    m_Initialized = FALSE;
}