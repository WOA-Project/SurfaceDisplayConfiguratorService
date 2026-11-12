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

#include <devpropdef.h>

#define DEFINE_DEVPROPKEY2(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) \
    EXTERN_C \
    const DEVPROPKEY DECLSPEC_SELECTANY name = {{l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}, pid}

// ============================================================================
// Physical Device Location
// ============================================================================

#ifndef DEVPKEY_Device_PhysicalDeviceLocation
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PhysicalDeviceLocation,
    0x540B947E,
    0x8B40,
    0x45BC,
    0xA8,
    0xA2,
    0x6A,
    0x0B,
    0x89,
    0x4C,
    0xBD,
    0xA2,
    9);
#endif

#ifndef DEVPKEY_Device_PhysicalDeviceLocationSpatial
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PhysicalDeviceLocationSpatial,
    0xE585291F,
    0x53AF,
    0x48AF,
    0x72,
    0x52,
    0x24,
    0x5A,
    0x68,
    0xC7,
    0x41,
    0x02,
    2);
#endif

#ifndef DEVPKEY_Device_PhysicalDeviceLocationJoint
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PhysicalDeviceLocationJoint,
    0x52ACC9AE,
    0x7867,
    0x445E,
    0xA9,
    0x15,
    0x5C,
    0x68,
    0xAE,
    0xEA,
    0xC8,
    0x63,
    2);
#endif

#ifndef DEVPKEY_Device_PhysicalDeviceLocationPanel
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PhysicalDeviceLocationPanel,
    0x24DAEB80,
    0x9823,
    0x4FF8,
    0x92,
    0xA6,
    0x3A,
    0x4C,
    0x61,
    0x89,
    0xF8,
    0x2C,
    2);
#endif

// ============================================================================
// Device Panel
// ============================================================================

#ifndef DEVPKEY_Device_PanelId
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PanelId,
    0x8DBC9C86,
    0x97A9,
    0x4BFF,
    0x9B,
    0xC6,
    0xBF,
    0xE9,
    0x5D,
    0x3E,
    0x6D,
    0xAD,
    2);
#endif

#ifndef DEVPKEY_Device_PanelGroup
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PanelGroup,
    0x8DBC9C86,
    0x97A9,
    0x4BFF,
    0x9B,
    0xC6,
    0xBF,
    0xE9,
    0x5D,
    0x3E,
    0x6D,
    0xAD,
    3);
#endif

#ifndef DEVPKEY_Device_PanelSide
DEFINE_DEVPROPKEY2(
    DEVPKEY_Device_PanelSide,
    0x8DBC9C86,
    0x97A9,
    0x4BFF,
    0x9B,
    0xC6,
    0xBF,
    0xE9,
    0x5D,
    0x3E,
    0x6D,
    0xAD,
    4);
#endif

typedef struct _ACPI_PLD_V2_BUFFER
{
    UINT32 Revision : 7;
    UINT32 IgnoreColor : 1;
    UINT32 Color : 24;
    UINT32 Width : 16;
    UINT32 Height : 16;
    UINT32 UserVisible : 1;
    UINT32 Dock : 1;
    UINT32 Lid : 1;
    UINT32 Panel : 3;
    UINT32 VerticalPosition : 2;
    UINT32 HorizontalPosition : 2;
    UINT32 Shape : 4;
    UINT32 GroupOrientation : 1;
    UINT32 GroupToken : 8;
    UINT32 GroupPosition : 8;
    UINT32 Bay : 1;
    UINT32 Ejectable : 1;
    UINT32 EjectionRequired : 1;
    UINT32 CabinetNumber : 8;
    UINT32 CardCageNumber : 8;
    UINT32 Reference : 1;
    UINT32 Rotation : 4;
    UINT32 Order : 5;
    UINT32 Reserved : 4;
    USHORT VerticalOffset;
    USHORT HorizontalOffset;
} ACPI_PLD_V2_BUFFER, *PACPI_PLD_V2_BUFFER;