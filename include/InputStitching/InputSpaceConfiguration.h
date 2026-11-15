#pragma once

#include <windows.h>

#include "InputSpacePolicy.h"

namespace InputStitching {

//
// Runtime configuration.
//
// Values are intentionally conservative
// and match current Surface Duo testing.
//
struct InputSpaceConfiguration
{
    //
    // Auto-publish rebuilt plans.
    //
    bool AutoPublish = true;

    //
    // Automatically rebuild when runtime
    // events occur.
    //
    bool AutoRebuild = true;

    //
    // Generate stitched topologies when
    // possible.
    //
    bool EnableStitching = true;

    //
    // Create a hinge gap region.
    //
    bool EnableGapRegion = true;

    //
    // Default hinge width in pixels.
    //
    LONG GapPixels = 84;

    //
    // Runtime default if posture cannot
    // be determined.
    //
    InputTopologyMode FallbackMode = InputTopologyMode::DualStitched;
};

//
// Global configuration.
//
extern InputSpaceConfiguration g_InputSpaceConfiguration;

//
// Loads configuration from persistent
// storage.
//
HRESULT
LoadInputSpaceConfiguration(InputSpaceConfiguration *Configuration);

//
// Saves configuration to persistent
// storage.
//
HRESULT
SaveInputSpaceConfiguration(const InputSpaceConfiguration *Configuration);

//
// Restores built-in defaults.
//
void
ResetInputSpaceConfiguration(InputSpaceConfiguration *Configuration);

} // namespace InputStitching