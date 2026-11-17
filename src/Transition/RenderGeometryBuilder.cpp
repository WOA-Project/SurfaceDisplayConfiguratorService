#include "pch.h"
#include "Transition/RenderGeometryBuilder.h"

#include <algorithm>

namespace Transition {

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

        geometry.Orientation = RenderOrientation::Unknown;

        geometry.OrientationCorrected = false;

        auto panelIt =
            std::find_if(CapturedDesktop.Panels.begin(), CapturedDesktop.Panels.end(), [&](const CapturedPanel &panel) {
                const LONG desktopWidth = panel.DesktopRect.right - panel.DesktopRect.left;

                const LONG desktopHeight = panel.DesktopRect.bottom - panel.DesktopRect.top;

                const LONG regionWidth = region.DesktopRect.right - region.DesktopRect.left;

                const LONG regionHeight = region.DesktopRect.bottom - region.DesktopRect.top;

                return desktopWidth == regionWidth && desktopHeight == regionHeight &&
                       panel.DesktopRect.left == region.DesktopRect.left &&
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
                geometry.Orientation = RenderOrientation::Portrait;

                geometry.RenderRect.left = geometry.DesktopRect.left;

                geometry.RenderRect.top = geometry.DesktopRect.top;

                geometry.RenderRect.right = geometry.DesktopRect.left + static_cast<LONG>(geometry.TextureWidth);

                geometry.RenderRect.bottom = geometry.DesktopRect.top + static_cast<LONG>(geometry.TextureHeight);
            }
            else
            {
                geometry.Orientation = RenderOrientation::Landscape;

                geometry.RenderRect = geometry.DesktopRect;
            }
        }

        Result.emplace_back(std::move(geometry));
    }

    return S_OK;
}

} // namespace Transition