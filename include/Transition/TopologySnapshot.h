#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace Transition {

enum class DisplayRole
{
    Unknown,

    Left,
    Right,

    Primary,
    Secondary,

    External
};

enum class DisplayRotation
{
    Identity = 0,
    Rotate90 = 1,
    Rotate180 = 2,
    Rotate270 = 3
};

struct DisplayRegion
{
    std::wstring Id;

    std::wstring Name;

    RECT DesktopRect{};

    //
    // Geometry that is actually rendered.
    // Animation code should use this rather
    // than DesktopRect.
    //
    RECT RenderRect{};

    DisplayRole Role = DisplayRole::Unknown;

    DisplayRotation Rotation = DisplayRotation::Identity;

    bool Internal = false;

    bool Primary = false;

    bool Enabled = true;

    UINT TextureWidth = 0;

    UINT TextureHeight = 0;
};

struct TopologySnapshot
{
    std::vector<DisplayRegion> Regions;

    bool IsValid() const { return !Regions.empty(); }

    void Reset() { Regions.clear(); }
};

} // namespace Transition