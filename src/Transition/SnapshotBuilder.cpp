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

        snapshot.Regions.emplace_back(std::move(region));
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

        snapshot.Regions.emplace_back(std::move(region));
    }

    return snapshot;
}

} // namespace Transition