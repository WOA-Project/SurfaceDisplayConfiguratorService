#pragma once

//
// Windows SDK compatibility layer.
//
// Enables building the transition framework
// against older SDKs such as 18362.
//

#include <sdkddkver.h>

#if NTDDI_VERSION <= NTDDI_WIN10_19H1

#    define TRANSITION_LEGACY_DCOMP 1

#else

#    define TRANSITION_LEGACY_DCOMP 0

#endif

//
// Missing annotations/macros.
//

#ifndef RETURN_IF_FAILED
#    define RETURN_IF_FAILED(expr) \
        do \
        { \
            HRESULT hr__ = (expr); \
            if (FAILED(hr__)) \
            { \
                return hr__; \
            } \
        } while (false)
#endif

//
// Older Windows SDK compatibility.
//
// Some newer DirectComposition samples
// assume APIs not available in 18362.
//

#if TRANSITION_LEGACY_DCOMP

#    define TRANSITION_DISABLE_OPACITY 1
#    define TRANSITION_DISABLE_TRANSFORMGROUPS 1

#endif