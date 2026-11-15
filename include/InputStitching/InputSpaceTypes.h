#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace InputStitching {

//
// High-level topology modes understood by the
// Input Space subsystem.
//

enum class InputTopologyMode
{
    //
    // Preserve the operating system's legacy
    // input space behaviour.
    //
    Legacy,

    //
    // Two panels participate in input but are
    // not stitched together.
    //
    DualIndependent,

    //
    // Two panels plus a custom hinge region.
    // Intended to activate HeatCore stitching.
    //
    DualStitched,

    //
    // Only the left panel participates in the
    // generated input topology.
    //
    SingleLeft,

    //
    // Only the right panel participates in the
    // generated input topology.
    //
    SingleRight,

    //
    // Reserved for caller-provided layouts.
    //
    Custom
};

//
// Semantic meaning of a region within a plan.
//

enum class RegionRole
{
    Display,

    Gap,

    Disabled,

    TouchOnly,

    PenOnly,

    Custom
};

//
// Canonical panel identity.
//
// These roles come from DisplayIdentityCache,
// not desktop coordinates.
//

enum class InternalPanelRole
{
    Unknown,

    LeftPanel,

    RightPanel,

    External
};

//
// Current internal panel topology.
//

enum class InternalPanelTopology
{
    None,

    Single,

    Dual
};

//
// Relative geometric arrangement of the
// participating panels.
//

enum class PanelArrangement
{
    Unknown,

    //
    // Left ↔ Right
    //
    Horizontal,

    //
    // Top ↕ Bottom
    //
    Vertical
};

//
// Diagnostics generated while building,
// validating, or publishing a plan.
//

struct Diagnostics
{
    HRESULT Status = S_OK;

    bool ValidationPassed = false;

    bool Published = false;

    std::wstring ValidationFailureReason;

    std::wstring Log;
};

//
// Human-readable metadata describing a region.
//

struct RegionMetadata
{
    RegionRole Role = RegionRole::Display;

    InternalPanelRole PanelRole = InternalPanelRole::Unknown;

    bool IsPrimary = false;

    bool IsInternal = false;

    std::wstring Name;

    std::wstring PanelId;

    RECT DesktopRect{};
};

} // namespace InputStitching