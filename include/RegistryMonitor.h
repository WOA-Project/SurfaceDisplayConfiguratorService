#pragma once

#include <windows.h>

class RegistryMonitor
{
public:
    RegistryMonitor();
    virtual ~RegistryMonitor();

    HRESULT Open(HKEY RootKey, PCWSTR SubKey);

    VOID Run();

    VOID Stop();

protected:
    virtual VOID OnRegistryChanged() = 0;

private:
    HKEY m_Key;
    HANDLE m_Event;
    BOOLEAN m_StopRequested;
};
