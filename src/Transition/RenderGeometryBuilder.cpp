#include "pch.h"
#include "Transition/RenderGeometryBuilder.h"

#include <algorithm>

namespace Transition {

namespace {

static float
ToDegrees(DisplayRotation Rotation)
{
    switch (Rotation)
    {
    case DisplayRotation::Rotate90:
        return 90.0f;

    case DisplayRotation::Rotate180:
        return 180.0f;

    case DisplayRotation::Rotate270:
        return 270.0f;

    default:
        return 0.0f;
    }
}

} // namespace

HRESULT
RenderGeometryBuilder::Build(
    const TopologySnapshot &Snapshot,
    const CapturedDesktop &CapturedDesktop,
    std::vector<RenderGeometry> &Result)
{
    Result.clear();

    Result.reserve(Snapshot.Regions.size());

    for (const auto &region : Snapshot.Regions)
    {
        RenderGeometry geometry{};

        geometry.Id = region.Id;

        geometry.DesktopRect = region.DesktopRect;

        geometry.RenderRect = region.DesktopRect;

        geometry.RotationDegrees = ToDegrees(region.Rotation);

        geometry.Orientation = RenderOrientation::Unknown;

        geometry.OrientationCorrected = false;

        auto panelIt = std::find_if(
            CapturedDesktop.Panels.begin(), CapturedDesktop.Panels.end(), [&](const CapturedPanel &panel) {
                return panel.DesktopRect.left == region.DesktopRect.left &&
                       panel.DesktopRect.top == region.DesktopRect.top;
            });

        if (panelIt != CapturedDesktop.Panels.end())
        {
            geometry.TextureWidth = panelIt->TextureWidth;

            geometry.TextureHeight = panelIt->TextureHeight;

            const LONG desktopWidth = geometry.DesktopRect.right - geometry.DesktopRect.left;

            const LONG desktopHeight = geometry.DesktopRect.bottom - geometry.DesktopRect.top;

            const bool orientationMismatch = desktopWidth == static_cast<LONG>(geometry.TextureHeight) &&
                                             desktopHeight == static_cast<LONG>(geometry.TextureWidth);

            geometry.OrientationCorrected = orientationMismatch;

            if (orientationMismatch)
            {
                geometry.RenderRect.left = geometry.DesktopRect.left;

                geometry.RenderRect.top = geometry.DesktopRect.top;

                geometry.RenderRect.right = geometry.RenderRect.left + static_cast<LONG>(geometry.TextureWidth);

                geometry.RenderRect.bottom = geometry.RenderRect.top + static_cast<LONG>(geometry.TextureHeight);

                geometry.Orientation = RenderOrientation::Portrait;
            }
            else
            {
                geometry.RenderRect = geometry.DesktopRect;

                geometry.Orientation = RenderOrientation::Landscape;
            }

            geometry.PivotX = static_cast<float>(geometry.TextureWidth) * 0.5f;

            geometry.PivotY = static_cast<float>(geometry.TextureHeight) * 0.5f;
        }

        Result.emplace_back(std::move(geometry));
    }

    return S_OK;
}

} // namespace Transition