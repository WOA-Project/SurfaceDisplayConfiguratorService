#pragma once

#include "InputStitching\InputSpaceManager.h"

namespace InputStitching {

//
// Global Input Space subsystem instance.
//
// The subsystem is intentionally modeled after
// the existing display-management components:
//
//     Policy
//        ↓
//     Builder
//        ↓
//     Validation
//        ↓
//     Publisher
//        ↓
//     NtConfigureInputSpace
//
// Typical usage:
//
//     InputSpacePolicy policy{};
//
//     g_InputSpaceManager.ApplyPolicy(policy);
//     g_InputSpaceManager.Publish();
//

extern InputSpaceManager g_InputSpaceManager;

} // namespace InputStitching