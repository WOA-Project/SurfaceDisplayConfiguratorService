#include "pch.h"

#include "RegistryMonitor.h"

RegistryMonitor::RegistryMonitor() : m_Key(nullptr), m_Event(nullptr), m_StopRequested(FALSE) {}

RegistryMonitor::~RegistryMonitor()
{
    Stop();

    if (m_Event)
    {
        CloseHandle(m_Event);
    }

    if (m_Key)
    {
        RegCloseKey(m_Key);
    }
}

HRESULT
RegistryMonitor::Open(HKEY RootKey, PCWSTR SubKey)
{
    LONG Status = RegOpenKeyEx(RootKey, SubKey, 0, KEY_NOTIFY | KEY_READ, &m_Key);

    if (Status != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(Status);
    }

    m_Event = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (!m_Event)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return ERROR_SUCCESS;
}

VOID
RegistryMonitor::Stop()
{
    m_StopRequested = TRUE;

    if (m_Event)
    {
        SetEvent(m_Event);
    }
}

VOID
RegistryMonitor::Run()
{
    while (!m_StopRequested)
    {
        OnRegistryChanged();

        RegNotifyChangeKeyValue(m_Key, FALSE, REG_NOTIFY_CHANGE_LAST_SET, m_Event, TRUE);

        DWORD Result = WaitForSingleObject(m_Event, INFINITE);

        if (Result != WAIT_OBJECT_0)
        {
            break;
        }

        ResetEvent(m_Event);
    }
}