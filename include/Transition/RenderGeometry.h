#pragma once

#include <windows.h>

#include <string>

namespace Transition {

enum class RenderOrientation
{
    Unknown,

    Landscape,

    Portrait
};

struct RenderGeometry
{
    //
    // Stable identity across topology,
    // capture and animation systems.
    //

    std::wstring Id;

    //
    // Desktop-space rectangle reported
    // by the topology model.
    //

    RECT DesktopRect{};

    //
    // Actual rendered geometry used by
    // animation and visual placement.
    //

    RECT RenderRect{};

    //
    // Captured content dimensions.
    //

    UINT TextureWidth = 0;

    UINT TextureHeight = 0;

    //
    // Render-space rotation.
    //

    float RotationDegrees = 0.0f;

    //
    // Texture-local pivot.
    //

    float PivotX = 0.0f;

    float PivotY = 0.0f;

    RenderOrientation Orientation = RenderOrientation::Unknown;

    bool OrientationCorrected = false;

    LONG DesktopWidth() const noexcept { return DesktopRect.right - DesktopRect.left; }

    LONG DesktopHeight() const noexcept { return DesktopRect.bottom - DesktopRect.top; }

    LONG RenderWidth() const noexcept { return RenderRect.right - RenderRect.left; }

    LONG RenderHeight() const noexcept { return RenderRect.bottom - RenderRect.top; }
};

} // namespace Transition