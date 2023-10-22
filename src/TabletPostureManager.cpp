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

HRESULT WINAPI
_RegSetKeyValue(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, DWORD dwType, BYTE *lpData, DWORD cbData)
{
    HRESULT status;

    if (lpSubKey && *lpSubKey)
    {
        status = RegCreateKeyEx(HKEY_CURRENT_USER, lpSubKey, NULL, NULL, NULL, 2, NULL, &hKey, NULL);
        if (SUCCEEDED(status))
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

    if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\Setup"), NULL, KEY_WRITE, &key)))
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
    DWORD TabletPostureSize = sizeof(BYTE);
    WNF_CHANGE_STAMP TabletPostureChangeStamp;

    if (IsOOBEInProgress())
    {
        return FALSE;
    }

    NTSTATUS status = NtQueryWnfStateData(
        &WNF_TMCN_ISTABLETPOSTURE, nullptr, nullptr, &TabletPostureChangeStamp, &TabletPosture, &TabletPostureSize);

    if (FAILED(status) || TabletPosture != (BYTE)state)
    {
        TabletPosture = (BYTE)state;
        status = RtlPublishWnfStateData(WNF_TMCN_ISTABLETPOSTURE, nullptr, &TabletPosture, sizeof(BYTE), nullptr);
    }

    return SUCCEEDED(status);
}

BOOL WINAPI
SetTabletModeState(BOOLEAN state)
{
    BYTE TabletMode = (BYTE)state;
    DWORD TabletModeSize = sizeof(BYTE);
    WNF_CHANGE_STAMP TabletModeChangeStamp;

    if (IsOOBEInProgress())
    {
        return FALSE;
    }

    NTSTATUS status = NtQueryWnfStateData(
        &WNF_TMCN_ISTABLETMODE, nullptr, nullptr, &TabletModeChangeStamp, &TabletMode, &TabletModeSize);

    if (FAILED(status) || TabletMode != (BYTE)state)
    {
        TabletMode = (BYTE)state;
        status = RtlPublishWnfStateData(WNF_TMCN_ISTABLETMODE, nullptr, &TabletMode, sizeof(BYTE), nullptr);
    }

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

    HRESULT status = RegGetValue(
        HKEY_CURRENT_USER,
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
        _T("TabletPostureTaskbar"),
        REG_DWORD,
        NULL,
        &pvData,
        &pcbData);

    if (FAILED(status) || pvData != (BYTE)state)
    {
        pvData = (BYTE)state;
        status = _RegSetKeyValue(
            HKEY_CURRENT_USER,
            _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
            _T("TabletPostureTaskbar"),
            REG_DWORD,
            (PBYTE)&pvData,
            pcbData);
    }

    return SUCCEEDED(status);
}