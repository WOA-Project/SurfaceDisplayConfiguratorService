#include "pch.h"

#include "Transition/SnapshotBuilder.h"

#include <InputStitching/InputSpaceTypes.h>

namespace Transition {

static DisplayRotation
ToRotation(DWORD Orientation)
{
    switch (Orientation)
    {
    case DMDO_90:
        return DisplayRotation::Rotate90;

    case DMDO_180:
        return DisplayRotation::Rotate180;

    case DMDO_270:
        return DisplayRotation::Rotate270;

    default:
        return DisplayRotation::Identity;
    }
}

TopologySnapshot
SnapshotBuilder::BuildTopologySnapshot(
    const wchar_t *LeftPanelId,
    const wchar_t *RightPanelId,
    const DEVMODE &LeftDevMode,
    const DEVMODE &RightDevMode,
    bool LeftEnabled,
    bool RightEnabled)
{
    TopologySnapshot snapshot;

    if (LeftEnabled)
    {
        DisplayRegion region;

        region.Id = LeftPanelId;

        region.Role = DisplayRole::Left;

        region.Enabled = true;

        region.Rotation = ToRotation(LeftDevMode.dmDisplayOrientation);

        region.DesktopRect = {
            LeftDevMode.dmPosition.x,
            LeftDevMode.dmPosition.y,
            LeftDevMode.dmPosition.x + static_cast<LONG>(LeftDevMode.dmPelsWidth),
            LeftDevMode.dmPosition.y + static_cast<LONG>(LeftDevMode.dmPelsHeight)};

        snapshot.Regions.push_back(std::move(region));
    }

    if (RightEnabled)
    {
        DisplayRegion region;

        region.Id = RightPanelId;

        region.Role = DisplayRole::Right;

        region.Enabled = true;

        region.Rotation = ToRotation(RightDevMode.dmDisplayOrientation);

        region.DesktopRect = {
            RightDevMode.dmPosition.x,
            RightDevMode.dmPosition.y,
            RightDevMode.dmPosition.x + static_cast<LONG>(RightDevMode.dmPelsWidth),
            RightDevMode.dmPosition.y + static_cast<LONG>(RightDevMode.dmPelsHeight)};

        snapshot.Regions.push_back(std::move(region));
    }

    return snapshot;
}

TopologySnapshot
SnapshotBuilder::Build(const InputStitching::Plan &Plan)
{
    TopologySnapshot snapshot;

    int i = 0;
    for (const auto &metadata : Plan.Metadata)
    {
        auto planRegion = Plan.Regions[i];
        i++;

        if (metadata.Role != InputStitching::RegionRole::Display)
        {
            continue;
        }

        DisplayRegion region;

        region.Id = metadata.PanelId;

        region.Name = metadata.Name;

        region.DesktopRect = metadata.DesktopRect;

        region.Internal = metadata.IsInternal;

        region.Primary = metadata.IsPrimary;

        switch (metadata.PanelRole)
        {
        case InputStitching::InternalPanelRole::LeftPanel:

            region.Role = DisplayRole::Left;

            break;

        case InputStitching::InternalPanelRole::RightPanel:

            region.Role = DisplayRole::Right;

            break;

        default:

            region.Role = DisplayRole::Unknown;

            break;
        }

        switch (planRegion.Rotation)
        {
        case 1:
            region.Rotation = DisplayRotation::Rotate90;
            break;

        case 2:
            region.Rotation = DisplayRotation::Rotate180;
            break;

        case 3:
            region.Rotation = DisplayRotation::Rotate270;
            break;

        default:
            region.Rotation = DisplayRotation::Identity;
            break;
        }

        snapshot.Regions.emplace_back(std::move(region));
    }

    return snapshot;
}

} // namespace Transition