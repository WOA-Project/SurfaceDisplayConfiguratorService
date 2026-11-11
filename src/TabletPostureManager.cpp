/*
 * Copyright (c) 2022-2023 The DuoWOA authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "pch.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Windows.h>
#include <initguid.h>
#include <Devpkey.h>
#include <SetupAPI.h>
#include <strsafe.h>
#include <cfgmgr32.h>
#include <winternl.h>
#include "TabletPostureManager.h"

ULONG64 WNF_TMCN_ISTABLETPOSTURE = 0x0F850339A3BC1035;
ULONG64 WNF_TMCN_ISTABLETMODE = 0x0F850339A3BC0835;

extern "C" {

typedef struct _WNF_TYPE_ID
{
    GUID TypeId;
} WNF_TYPE_ID, *PWNF_TYPE_ID;
typedef const WNF_TYPE_ID *PCWNF_TYPE_ID;

typedef ULONG WNF_CHANGE_STAMP, *PWNF_CHANGE_STAMP;
typedef ULONGLONG WNF_STATE_NAME, *PWNF_STATE_NAME;

NTSTATUS NTAPI
NtQueryWnfStateData(
    _In_ PWNF_STATE_NAME StateName,
    _In_opt_ PCWNF_TYPE_ID TypeId,
    _In_opt_ const VOID *ExplicitScope,
    _Out_ PWNF_CHANGE_STAMP ChangeStamp,
    _Out_writes_bytes_to_opt_(*BufferSize, *BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize);

NTSTATUS NTAPI
RtlPublishWnfStateData(
    _In_ WNF_STATE_NAME StateName,
    _In_opt_ PCWNF_TYPE_ID TypeId,
    _In_reads_bytes_opt_(Length) const VOID *Buffer,
    _In_opt_ ULONG Length,
    _In_opt_ const PVOID ExplicitScope);
}

typedef NTSTATUS(NTAPI *PRTL_SUBSCRIBE_WNF_STATE_CHANGE_NOTIFICATION)(
    PVOID *Subscription,
    WNF_STATE_NAME StateName,
    ULONG ChangeStamp,
    PVOID Callback,
    PVOID CallbackContext,
    PVOID TypeId,
    ULONG SerializationGroup);

typedef NTSTATUS(NTAPI *PRTL_UNSUBSCRIBE_WNF_STATE_CHANGE_NOTIFICATION)(PVOID Subscription);

volatile BYTE g_CurrentTabletPosture = 0xFF;
volatile BYTE g_CurrentTabletMode = 0xFF;

PVOID g_TabletPostureSubscription = nullptr;
PVOID g_TabletModeSubscription = nullptr;

volatile DWORD g_CurrentTabletPostureTaskbar = 0xFFFFFFFF;

HANDLE g_TaskbarMonitorThread = nullptr;
BOOLEAN g_StopTaskbarMonitor = FALSE;

DWORD WINAPI
TaskbarRegistryMonitorThread(LPVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    HKEY key;

    if (RegOpenKeyEx(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
            0,
            KEY_NOTIFY | KEY_QUERY_VALUE,
            &key) != ERROR_SUCCESS)
    {
        return 1;
    }

    HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    while (!g_StopTaskbarMonitor)
    {
        DWORD value = 0;
        DWORD size = sizeof(value);

        if (RegGetValue(
                HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                L"TabletPostureTaskbar",
                RRF_RT_REG_DWORD,
                nullptr,
                &value,
                &size) == ERROR_SUCCESS)
        {
            g_CurrentTabletPostureTaskbar = value;
        }

        RegNotifyChangeKeyValue(key, FALSE, REG_NOTIFY_CHANGE_LAST_SET, eventHandle, TRUE);

        WaitForSingleObject(eventHandle, INFINITE);

        printf("Registry: TabletPostureTaskbar=%lu\n", g_CurrentTabletPostureTaskbar);
    }

    CloseHandle(eventHandle);
    RegCloseKey(key);

    return 0;
}

NTSTATUS CALLBACK
TabletPostureWnfCallback(
    WNF_STATE_NAME StateName,
    WNF_CHANGE_STAMP ChangeStamp,
    PWNF_TYPE_ID TypeId,
    PVOID CallbackContext,
    const VOID *Buffer,
    ULONG BufferSize)
{
    UNREFERENCED_PARAMETER(ChangeStamp);
    UNREFERENCED_PARAMETER(TypeId);
    UNREFERENCED_PARAMETER(CallbackContext);

    if (!Buffer || BufferSize < sizeof(BYTE))
    {
        return 0;
    }

    BYTE value = *(const BYTE *)Buffer;

    if (StateName == WNF_TMCN_ISTABLETPOSTURE)
    {
        g_CurrentTabletPosture = value;

        printf("WNF: TabletPosture=%u\n", value);
    }
    else if (StateName == WNF_TMCN_ISTABLETMODE)
    {
        g_CurrentTabletMode = value;

        printf("WNF: TabletMode=%u\n", value);
    }

    return 0;
}

BOOL WINAPI
InitializeTabletPostureSubscriptions()
{
    auto Subscribe = (PRTL_SUBSCRIBE_WNF_STATE_CHANGE_NOTIFICATION)GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"), "RtlSubscribeWnfStateChangeNotification");

    if (!Subscribe)
    {
        return FALSE;
    }

    BYTE Value;
    ULONG Size;
    WNF_CHANGE_STAMP Stamp;

    Value = 0;
    Size = sizeof(Value);

    NtQueryWnfStateData(&WNF_TMCN_ISTABLETPOSTURE, nullptr, nullptr, &Stamp, &Value, &Size);

    g_CurrentTabletPosture = Value;

    Value = 0;
    Size = sizeof(Value);

    NtQueryWnfStateData(&WNF_TMCN_ISTABLETMODE, nullptr, nullptr, &Stamp, &Value, &Size);

    g_CurrentTabletMode = Value;

    Subscribe(&g_TabletPostureSubscription, WNF_TMCN_ISTABLETPOSTURE, 0, TabletPostureWnfCallback, nullptr, nullptr, 0);

    Subscribe(&g_TabletModeSubscription, WNF_TMCN_ISTABLETMODE, 0, TabletPostureWnfCallback, nullptr, nullptr, 0);

    return TRUE;
}

VOID WINAPI
ShutdownTabletPostureSubscriptions()
{
    auto Unsubscribe = (PRTL_UNSUBSCRIBE_WNF_STATE_CHANGE_NOTIFICATION)GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"), "RtlUnsubscribeWnfStateChangeNotification");

    if (!Unsubscribe)
    {
        return;
    }

    if (g_TabletPostureSubscription)
    {
        Unsubscribe(g_TabletPostureSubscription);
        g_TabletPostureSubscription = nullptr;
    }

    if (g_TabletModeSubscription)
    {
        Unsubscribe(g_TabletModeSubscription);
        g_TabletModeSubscription = nullptr;
    }
}

HRESULT WINAPI
_RegSetKeyValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, DWORD dwType, BYTE *lpData, DWORD cbData)
{
    HRESULT status;

    if (lpSubKey && *lpSubKey)
    {
        status = RegCreateKeyEx(HKEY_CURRENT_USER, lpSubKey, NULL, NULL, NULL, 2, NULL, &hKey, NULL);
        if (!SUCCEEDED(status))
            return status;
    }
    else
    {
        hKey = HKEY_CURRENT_USER;
    }

    status = RegSetValueEx(hKey, lpValueName, NULL, dwType, lpData, cbData);

    if (hKey != HKEY_CURRENT_USER)
        RegCloseKey(hKey);

    return status;
}

BOOLEAN WINAPI
IsOOBEInProgress()
{
    DWORD oobeInProgress = 0;

    HKEY key;
    DWORD type = REG_DWORD, size = 8;

    if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\Setup"), NULL, KEY_READ, &key)))
    {
        RegQueryValueEx(key, _T("OOBEInProgress"), NULL, &type, (LPBYTE)&oobeInProgress, &size);
        RegCloseKey(key);
    }

    return oobeInProgress == 1;
}

BOOL WINAPI
SetTabletPostureState(BOOLEAN state)
{
    BYTE TabletPosture = (BYTE)state;

    // printf("Set Table Posture State Entry\n");

    if (IsOOBEInProgress())
    {
        printf("OOBE is in progress, aborting setting tablet posture state.\n");
        return FALSE;
    }

    if (g_CurrentTabletPosture == (BYTE)state)
    {
        return TRUE;
    }

    TabletPosture = (BYTE)state;
    NTSTATUS status = RtlPublishWnfStateData(WNF_TMCN_ISTABLETPOSTURE, nullptr, &TabletPosture, sizeof(BYTE), nullptr);

    return SUCCEEDED(status);
}

BOOL WINAPI
SetWallpaperSpanStyle()
{
    HRESULT status;

    const WCHAR *pvData = L"22";
    DWORD pcbData = ((DWORD)wcslen(pvData) + 1) * sizeof(WCHAR);

    status = _RegSetKeyValue(
        HKEY_CURRENT_USER, _T("Control Panel\\Desktop"), _T("WallpaperStyle"), REG_SZ, (PBYTE)pvData, pcbData);

    return SUCCEEDED(status);
}

BOOL WINAPI
SetTabletModeState(BOOLEAN state)
{
    BYTE TabletMode = (BYTE)state;

    if (IsOOBEInProgress())
    {
        return FALSE;
    }

    if (g_CurrentTabletMode == (BYTE)state)
    {
        return TRUE;
    }

    TabletMode = (BYTE)state;
    NTSTATUS status = RtlPublishWnfStateData(WNF_TMCN_ISTABLETMODE, nullptr, &TabletMode, sizeof(BYTE), nullptr);

    return SUCCEEDED(status);
}

BOOL WINAPI
SetTabletPostureTaskbarState(BOOLEAN state)
{
    DWORD pcbData = sizeof(DWORD);
    DWORD pvData = (BYTE)state;

    if (IsOOBEInProgress())
    {
        return FALSE;
    }

    if (g_CurrentTabletPostureTaskbar == (DWORD)state)
    {
        return TRUE;
    }

    pvData = (BYTE)state;
    HRESULT status = _RegSetKeyValue(
        HKEY_CURRENT_USER,
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
        _T("TabletPostureTaskbar"),
        REG_DWORD,
        (PBYTE)&pvData,
        pcbData);

    return SUCCEEDED(status);
}