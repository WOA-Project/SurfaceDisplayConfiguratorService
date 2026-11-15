#pragma once

#include "InputStitching\InputSpaceAbi.h"
#include "InputStitching\InputSpaceTypes.h"

#include <vector>

namespace InputStitching {

//
// Internal representation of a generated
// input space configuration.
//
// A Plan is built by InputSpaceBuilder,
// validated by InputSpaceValidation,
// then published by InputSpacePublisher.
//

struct Plan
{
    //
    // Kernel-facing INPUT_SPACE descriptor.
    //
    INPUT_SPACE Space{};

    //
    // Regions that will be passed to
    // NtConfigureInputSpace().
    //
    std::vector<INPUT_SPACE_REGION> Regions;

    //
    // Human-readable metadata for diagnostics.
    //
    std::vector<RegionMetadata> Metadata;

    //
    // Effective topology mode.
    //
    InputTopologyMode Mode = InputTopologyMode::Legacy;

    //
    // Indicates that a stitch region has
    // been successfully generated.
    //
    bool Stitched = false;

    //
    // Indicates that a custom gap region
    // exists in Regions.
    //
    bool GapEnabled = false;

    //
    // Effective gap width in desktop pixels.
    //
    LONG GapPixels = 0;

    //
    // Total number of display regions
    // participating in the plan.
    //
    UINT32 DisplayCount = 0;

    //
    // Number of internal Duo panels
    // participating in the plan.
    //
    UINT32 InternalDisplayCount = 0;

    //
    // Current arrangement of the
    // participating internal panels.
    //
    DeviceArrangement Arrangement = DeviceArrangement::Unknown;

    //
    // Internal panel topology.
    //
    InternalPanelTopology Topology = InternalPanelTopology::None;

    //
    // Build / validation / publish diagnostics.
    //
    Diagnostics Debug;

    //
    // Convenience helpers.
    //

    bool IsValid() const { return !Regions.empty(); }

    bool HasGapRegion() const { return GapEnabled; }

    bool HasDualInternalPanels() const { return Topology == InternalPanelTopology::Dual; }

    bool CanStitch() const { return Stitched && HasGapRegion() && HasDualInternalPanels(); }

    void Reset() { *this = Plan{}; }

    const RegionMetadata *FindPanel(InternalPanelRole Role) const
    {
        for (const auto &metadata : Metadata)
        {
            if (metadata.PanelRole == Role)
            {
                return &metadata;
            }
        }

        return nullptr;
    }
};

} // namespace InputStitching