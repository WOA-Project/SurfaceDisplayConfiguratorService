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

BOOL WINAPI
EnableTabletPosture()
{
    BYTE TabletPosture = 0;
    DWORD TabletPostureSize = 1;
    WNF_CHANGE_STAMP TabletPostureChangeStamp;

    NTSTATUS status = NtQueryWnfStateData(
        &WNF_TMCN_ISTABLETPOSTURE, nullptr, nullptr, &TabletPostureChangeStamp, &TabletPosture, &TabletPostureSize);

    if (SUCCEEDED(status) && TabletPosture != 1)
    {
        TabletPosture = 1;
        status = RtlPublishWnfStateData(WNF_TMCN_ISTABLETPOSTURE, nullptr, &TabletPosture, 1, nullptr);
    }

    return SUCCEEDED(status);
}

BOOL WINAPI
EnableTabletMode()
{
    BYTE TabletMode = 0;
    DWORD TabletModeSize = 1;
    WNF_CHANGE_STAMP TabletModeChangeStamp;

    NTSTATUS status = NtQueryWnfStateData(
        &WNF_TMCN_ISTABLETMODE, nullptr, nullptr, &TabletModeChangeStamp, &TabletMode, &TabletModeSize);

    if (SUCCEEDED(status) && TabletMode != 1)
    {
        TabletMode = 1;
        status = RtlPublishWnfStateData(WNF_TMCN_ISTABLETMODE, nullptr, &TabletMode, 1, nullptr);
    }

    return SUCCEEDED(status);
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

BOOL WINAPI
EnableTabletPostureTaskbar()
{
    DWORD pcbData = 4;
    int pvData = 0;

    HRESULT status = RegGetValue(
        HKEY_CURRENT_USER,
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
        _T("TabletPostureTaskbar"),
        REG_DWORD,
        NULL,
        &pvData,
        &pcbData);

    if (FAILED(status) || pvData != 1)
    {
        pvData = 1;
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