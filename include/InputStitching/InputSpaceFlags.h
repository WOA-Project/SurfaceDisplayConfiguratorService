#pragma once

#include "InputStitching\InputSpaceAbi.h"

namespace InputStitching {

//
// Known-safe region flags.
//
// Reverse engineered from:
//
//   win32kbase!CInputConfig::Validate
//
// Rules:
//
//   Exactly one of:
//
//      RegionDisplay
//      RegionCustom
//
//   must be present for every region.
//

enum RegionFlags : UINT32
{
    //
    // Primary/default region.
    //
    RegionPrimary = kRegionPrimary,

    //
    // Display-backed region.
    //
    RegionDisplay = kRegionDisplay,

    //
    // Caller-defined region.
    //
    // HeatCore appears to use the existence
    // of at least one custom region to enable
    // stitched input processing.
    //
    RegionCustom = kRegionCustom
};

//
// Safe flag builders
//

constexpr UINT32
DisplayRegion()
{
    return RegionDisplay;
}

constexpr UINT32
PrimaryDisplayRegion()
{
    return RegionDisplay | RegionPrimary;
}

constexpr UINT32
CustomRegion()
{
    return RegionCustom;
}

constexpr UINT32
PrimaryCustomRegion()
{
    return RegionCustom | RegionPrimary;
}

//
// Validation helpers
//

inline bool
IsDisplayRegion(UINT32 Flags)
{
    return (Flags & RegionDisplay) != 0;
}

inline bool
IsCustomRegion(UINT32 Flags)
{
    return (Flags & RegionCustom) != 0;
}

inline bool
IsPrimaryRegion(UINT32 Flags)
{
    return (Flags & RegionPrimary) != 0;
}

//
// Safe combination validation.
//
// Valid:
//
//   Display
//   Display | Primary
//   Custom
//   Custom | Primary
//
// Invalid:
//
//   0
//   Display | Custom
//   Display | Custom | Primary
//

inline bool
IsValidRegionFlags(UINT32 Flags)
{
    const bool display = (Flags & RegionDisplay) != 0;

    const bool custom = (Flags & RegionCustom) != 0;

    //
    // Exactly one of Display/Custom.
    //
    if (display == custom)
    {
        return false;
    }

    return true;
}

//
// Higher-level helpers.
//

inline bool
IsDisplayPrimary(UINT32 Flags)
{
    return IsDisplayRegion(Flags) && IsPrimaryRegion(Flags);
}

inline bool
IsCustomPrimary(UINT32 Flags)
{
    return IsCustomRegion(Flags) && IsPrimaryRegion(Flags);
}

} // namespace InputStitching