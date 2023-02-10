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
#pragma once

#define DEFINE_DEVPROPKEY2(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) \
    EXTERN_C \
    const DEVPROPKEY DECLSPEC_SELECTANY name = {{l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}, pid}

#ifndef DEVPKEY_Device_PanelId
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PanelId,
    0x8dbc9c86,
    0x97a9,
    0x4bff,
    0x9b,
    0xc6,
    0xbf,
    0xe9,
    0x5d,
    0x3e,
    0x6d,
    0xad,
    2); // DEVPROP_TYPE_STRING
#endif

#ifndef DEVPKEY_Device_PhysicalDeviceLocation
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PhysicalDeviceLocation,
    0x540b947e,
    0x8b40,
    0x45bc,
    0xa8,
    0xa2,
    0x6a,
    0x0b,
    0x89,
    0x4c,
    0xbd,
    0xa2,
    9); // DEVPROP_TYPE_BINARY
#endif

typedef struct _ACPI_PLD_V2_BUFFER
{
    UINT32 Revision : 7;
    UINT32 IgnoreColor : 1;
    UINT32 Color : 24;
    UINT32 Panel : 3;
    UINT32 CardCageNumber : 8;
    UINT32 Reference : 1;
    UINT32 Rotation : 4;
    UINT32 Order : 5;
    UINT32 Reserved : 4;
    USHORT VerticalOffset;
    USHORT HorizontalOffset;
} ACPI_PLD_V2_BUFFER, *PACPI_PLD_V2_BUFFER;