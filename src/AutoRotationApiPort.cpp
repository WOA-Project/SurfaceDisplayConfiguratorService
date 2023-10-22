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
#include "NtAlpc.h"
#include "AutoRotationApiPort.h"
#include <tchar.h>

//
// The handle to the auto rotation ALPC port
//
HANDLE PortHandle = NULL;

HRESULT
InitializeAutoRotationApiPort()
{
    HRESULT Status;
    UNICODE_STRING DestinationString;
    OBJECT_ATTRIBUTES ObjectAttribs;
    ALPC_PORT_ATTRIBUTES PortAttribs = {0};

    //
    // Initialize ALPC Port. This will decide the way we do screen flip
    //
    RtlZeroMemory(&ObjectAttribs, sizeof(ObjectAttribs));
    ObjectAttribs.Length = sizeof(ObjectAttribs);

    RtlZeroMemory(&PortAttribs, sizeof(PortAttribs));
    PortAttribs.MaxMessageLength = 56;

    RtlInitUnicodeString(&DestinationString, _T("\\RPC Control\\AutoRotationApiPort"));

#pragma warning(disable : 6387)
    Status = NtAlpcConnectPort(
        &PortHandle,
        &DestinationString,
        &ObjectAttribs,
        &PortAttribs,
        ALPC_MSGFLG_SYNC_REQUEST,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
#pragma warning(default : 6387)

    if (FAILED(Status))
    {
        // Can't initialize critical section; skip Auto rotation API
        PortHandle = NULL;
    }

    return Status;
}

//
// Subject: Notify auto rotation with the following current auto rotation settings
//			using ALPC port
//
// Parameters:
//
//			   Handle: the ALPC Port Handle
//
//             Orientation:
//             - DMDO_270     (Portrait)
//             - DMDO_90      (Portrait flipped)
//             - DMDO_180     (Landscape)
//             - DMDO_DEFAULT (Landscape flipped)
//
// Returns: HRESULT
//
HRESULT
NotifyAutoRotationAlpcPortOfOrientationChange(INT Orientation)
{
    ROTATION_COMMAND_MESSAGE RotationCommandMessage;
    HRESULT Status;

    if (PortHandle == NULL)
    {
        Status = InitializeAutoRotationApiPort();
        if (FAILED(Status))
        {
            return Status;
        }
    }

    RtlZeroMemory(&RotationCommandMessage, sizeof(RotationCommandMessage));

    RotationCommandMessage.PortMessage.u1.s1.DataLength = sizeof(RotationCommandMessage.RotationMessage);
    RotationCommandMessage.PortMessage.u1.s1.TotalLength = sizeof(RotationCommandMessage);
    RotationCommandMessage.PortMessage.u2.s2.Type = 1;

    RotationCommandMessage.RotationMessage.Type = 2;
    RotationCommandMessage.RotationMessage.Orientation = Orientation;

    Status = NtAlpcSendWaitReceivePort(PortHandle, 0, (PVOID)&RotationCommandMessage, NULL, NULL, NULL, NULL, NULL);

    return Status;
}