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
    std::wstring Id;

    RECT DesktopRect{};

    RECT RenderRect{};

    UINT TextureWidth = 0;

    UINT TextureHeight = 0;

    RenderOrientation Orientation = RenderOrientation::Unknown;

    bool OrientationCorrected = false;

    LONG DesktopWidth() const noexcept { return DesktopRect.right - DesktopRect.left; }

    LONG DesktopHeight() const noexcept { return DesktopRect.bottom - DesktopRect.top; }

    LONG RenderWidth() const noexcept { return RenderRect.right - RenderRect.left; }

    LONG RenderHeight() const noexcept { return RenderRect.bottom - RenderRect.top; }

    bool IsOrientationMismatch() const noexcept
    {
        return DesktopWidth() == static_cast<LONG>(TextureHeight) && DesktopHeight() == static_cast<LONG>(TextureWidth);
    }
};

} // namespace Transition