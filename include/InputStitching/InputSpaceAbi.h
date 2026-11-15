#pragma once

#include <windows.h>

namespace InputStitching {

//
// Region flags discovered through reverse-engineering
// of win32kbase!CInputConfig::Validate and related code.
//

constexpr UINT32 kRegionPrimary = 0x00000001;
constexpr UINT32 kRegionDisplay = 0x00000002;
constexpr UINT32 kRegionCustom = 0x00000004;

//
// INPUT_SPACE
//
// User-mode descriptor passed to
// win32u!NtConfigureInputSpace.
//
// Notes:
//
//  Reserved must be zero.
//
//  The legacy input space uses the reserved
//  Id value { LowPart = 0, HighPart = -2 }.
//
// Size: 0x0C bytes
//

struct INPUT_SPACE
{
    LUID Id;         // +0x00
    UINT32 Reserved; // +0x08
};

static_assert(sizeof(INPUT_SPACE) == 0x0C, "INPUT_SPACE must be 0x0C bytes");

//
// INPUT_SPACE_REGION
//
// Reverse engineered from the head of
// win32k's CInputSpaceRegion.
//
// Size: 0xC8 bytes
//

struct INPUT_SPACE_REGION
{
    //
    // Desktop-space rectangle.
    //
    RECT Rect; // +0x00

    //
    // kRegion*
    //
    UINT32 Flags; // +0x10

    //
    // Effective monitor DPI.
    //
    UINT16 Dpi;    // +0x14
    UINT16 DpiAlt; // +0x16

    //
    // DISPLAYCONFIG source.
    //
    LUID SourceAdapterId; // +0x18
    UINT32 SourceId;      // +0x20

    //
    // DISPLAYCONFIG target.
    //
    LUID TargetAdapterId; // +0x24
    UINT32 TargetId;      // +0x2C

    //
    // Populated internally by win32k.
    //
    BYTE KernelData[0x74]; // +0x30

    //
    // DISPLAYCONFIG_ROTATION
    //
    UINT32 Rotation; // +0xA4

    //
    // Display mode rectangle.
    //
    RECT ModeRect; // +0xA8

    //
    // Scaling rectangle.
    //
    RECT ScalingRect; // +0xB8
};

static_assert(sizeof(INPUT_SPACE_REGION) == 0xC8, "INPUT_SPACE_REGION must be 0xC8 bytes");

static_assert(offsetof(INPUT_SPACE_REGION, Flags) == 0x10, "Flags offset mismatch");

static_assert(offsetof(INPUT_SPACE_REGION, Dpi) == 0x14, "Dpi offset mismatch");

static_assert(offsetof(INPUT_SPACE_REGION, SourceAdapterId) == 0x18, "SourceAdapterId offset mismatch");

static_assert(offsetof(INPUT_SPACE_REGION, SourceId) == 0x20, "SourceId offset mismatch");

static_assert(offsetof(INPUT_SPACE_REGION, TargetAdapterId) == 0x24, "TargetAdapterId offset mismatch");

static_assert(offsetof(INPUT_SPACE_REGION, TargetId) == 0x2C, "TargetId offset mismatch");

static_assert(offsetof(INPUT_SPACE_REGION, Rotation) == 0xA4, "Rotation offset mismatch");

static_assert(offsetof(INPUT_SPACE_REGION, ModeRect) == 0xA8, "ModeRect offset mismatch");

static_assert(offsetof(INPUT_SPACE_REGION, ScalingRect) == 0xB8, "ScalingRect offset mismatch");

//
// NtConfigureInputSpace
//
// Exported by win32u.dll.
//

using PFN_NtConfigureInputSpace =
    BOOL(WINAPI *)(const INPUT_SPACE *Space, const INPUT_SPACE_REGION *Regions, UINT32 RegionCount);

} // namespace InputStitching