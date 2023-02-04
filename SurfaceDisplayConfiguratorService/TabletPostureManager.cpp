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
    } WNF_TYPE_ID, * PWNF_TYPE_ID;
    typedef const WNF_TYPE_ID* PCWNF_TYPE_ID;

    typedef ULONG WNF_CHANGE_STAMP, * PWNF_CHANGE_STAMP;
    typedef ULONGLONG WNF_STATE_NAME, * PWNF_STATE_NAME;

    NTSTATUS NTAPI
        NtQueryWnfStateData(
            _In_ PWNF_STATE_NAME StateName,
            _In_opt_ PCWNF_TYPE_ID TypeId,
            _In_opt_ const VOID* ExplicitScope,
            _Out_ PWNF_CHANGE_STAMP ChangeStamp,
            _Out_writes_bytes_to_opt_(*BufferSize, *BufferSize) PVOID Buffer,
            _Inout_ PULONG BufferSize);

    NTSTATUS NTAPI
        RtlPublishWnfStateData(
            _In_ WNF_STATE_NAME StateName,
            _In_opt_ PCWNF_TYPE_ID TypeId,
            _In_reads_bytes_opt_(Length) const VOID* Buffer,
            _In_opt_ ULONG Length,
            _In_opt_ const PVOID ExplicitScope);
}

BOOL WINAPI EnableTabletPosture()
{
    BYTE TabletPosture[4];
    DWORD TabletPostureSize = 1;
    WNF_CHANGE_STAMP TabletPostureChangeStamp;

    NTSTATUS status = NtQueryWnfStateData(&WNF_TMCN_ISTABLETPOSTURE, nullptr, nullptr, &TabletPostureChangeStamp,
        TabletPosture, &TabletPostureSize);

    if (status == ERROR_SUCCESS && TabletPosture[0] != 1)
    {
        TabletPosture[0] = 1;
        status = RtlPublishWnfStateData(WNF_TMCN_ISTABLETPOSTURE, nullptr, TabletPosture, 1, nullptr);
    }

    return status == ERROR_SUCCESS;
}

BOOL WINAPI EnableTabletMode()
{
    BYTE TabletMode[4];
    DWORD TabletModeSize = 1;
    WNF_CHANGE_STAMP TabletModeChangeStamp;

    NTSTATUS status = NtQueryWnfStateData(&WNF_TMCN_ISTABLETMODE, nullptr, nullptr, &TabletModeChangeStamp,
        TabletMode, &TabletModeSize);

    if (status == ERROR_SUCCESS && TabletMode[0] != 1)
    {
        TabletMode[0] = 1;
        status = RtlPublishWnfStateData(WNF_TMCN_ISTABLETMODE, nullptr, TabletMode, 1, nullptr);
    }

    return status == ERROR_SUCCESS;
}

NTSTATUS WINAPI _RegSetKeyValue(
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValueName,
    DWORD dwType,
    BYTE* lpData,
    DWORD cbData)
{
    NTSTATUS status;

    if (lpSubKey && *lpSubKey)
    {
        status = RegCreateKeyEx(HKEY_CURRENT_USER, lpSubKey, NULL, NULL, NULL, 2, NULL, &hKey, NULL);
        if (status)
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

BOOL WINAPI EnableTabletPostureTaskbar()
{
    DWORD pcbData = 4;
    int pvData = 0;

    NTSTATUS status = RegGetValue(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
        L"TabletPostureTaskbar",
        REG_DWORD,
        NULL,
        &pvData,
        &pcbData);

    if (status != ERROR_SUCCESS || pvData != 1)
    {
        pvData = 1;
        status = _RegSetKeyValue(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
            L"TabletPostureTaskbar",
            REG_DWORD,
            (PBYTE)&pvData,
            pcbData);
    }

    return status == ERROR_SUCCESS;
}