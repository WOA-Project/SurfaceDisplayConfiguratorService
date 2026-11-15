#pragma once

#include "InputStitching\InputSpaceTypes.h"

namespace InputStitching {

//
// High-level policy describing how the
// Input Space subsystem should construct
// and publish an input topology.
//
struct InputSpacePolicy
{
    //
    // Requested topology mode.
    //
    InputTopologyMode Mode = InputTopologyMode::Legacy;

    //
    // Enable HeatCore stitching.
    //
    // When enabled, BuildPlan() may append
    // a custom hinge region between the
    // left and right panels.
    //
    bool EnableStitching = false;

    //
    // Permit creation of a custom gap
    // region between internal panels.
    //
    bool EnableGapRegion = false;

    //
    // Gap width in desktop pixels.
    //
    // A value of zero allows the gap
    // calculator to derive the gap from
    // current display topology.
    //
    LONG GapPixels = 0;

    //
    // Touch participation policy.
    //
    bool EnableDisplay1Input = true;
    bool EnableDisplay2Input = true;

    //
    // Pen participation policy.
    //
    bool EnableDisplay1Pen = true;
    bool EnableDisplay2Pen = true;

    //
    // Publish through
    // NtConfigureInputSpace.
    //
    // Useful for diagnostics and A/B
    // testing plan generation without
    // affecting the system.
    //
    bool PublishToSystem = true;
};

} // namespace InputStitching