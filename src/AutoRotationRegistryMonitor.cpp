#include "pch.h"

#include "AutoRotationRegistryMonitor.h"

#include "AutoRotateContext.h"

constexpr PCWSTR WINDOWS_AUTO_ROTATION_KEY_PATH = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AutoRotation";

HRESULT
AutoRotationRegistryMonitor::Initialize()
{
    return Open(HKEY_LOCAL_MACHINE, WINDOWS_AUTO_ROTATION_KEY_PATH);
}

VOID
AutoRotationRegistryMonitor::OnRegistryChanged()
{
    HKEY Key = nullptr;

    LONG Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINDOWS_AUTO_ROTATION_KEY_PATH, 0, KEY_READ, &Key);

    if (Result != ERROR_SUCCESS)
    {
        return;
    }

    DWORD Type = REG_DWORD;
    DWORD Size = sizeof(DWORD);
    DWORD Enabled = 0;

    RegQueryValueEx(Key, L"Enable", nullptr, &Type, reinterpret_cast<LPBYTE>(&Enabled), &Size);

    RegCloseKey(Key);

    g_AutoRotateContext.AutoRotationEnabled = (Enabled != 0);
}
