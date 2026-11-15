#pragma once

#include "InputStitching\InputSpaceAbi.h"
#include "InputStitching\InputSpaceTypes.h"

#include <string>
#include <vector>

struct DisplayIdentity;

namespace InputStitching {

//
// Internal display representation used by
// InputSpaceBuilder.
//
// DisplayEntry represents a single active
// DISPLAYCONFIG path translated into an
// INPUT_SPACE_REGION-compatible form.
//

struct DisplayEntry
{
    //
    // Input-space region generated from the
    // DISPLAYCONFIG source/target path.
    //
    INPUT_SPACE_REGION Region{};

    //
    // Friendly monitor name.
    //
    std::wstring Name;

    //
    // Platform panel identifier.
    //
    // Resolved using:
    //
    //   GetPanelIdForDisplayDevice()
    //
    std::wstring PanelId;

    //
    // Matching identity from the global
    // DisplayIdentityCache.
    //
    const DisplayIdentity *Identity = nullptr;

    //
    // Internal Duo panel?
    //
    bool Internal = false;

    //
    // Canonical left/right role.
    //
    InternalPanelRole Role = InternalPanelRole::Unknown;
};

//
// Enumerates active displays and converts
// them into DisplayEntry instances.
//
// Responsibilities:
//
//   QueryDisplayConfig()
//
//   DPI discovery
//
//   Rotation discovery
//
//   PanelId resolution
//
//   DisplayIdentityCache lookup
//
//   Deterministic ordering
//
// Ordering:
//
//   LeftPanel
//   RightPanel
//   External displays
//
// independent of rotation, desktop origin,
// monitor numbering, or primary-display state.
//

HRESULT
EnumerateDisplays(std::vector<DisplayEntry> *displays);

} // namespace InputStitching