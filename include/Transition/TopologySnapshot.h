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
    // Topology geometry only.
    // No render/capture geometry belongs here.
    //
    DisplayRole Role = DisplayRole::Unknown;

    DisplayRotation Rotation = DisplayRotation::Identity;

    bool Internal = false;

    bool Primary = false;

    bool Enabled = true;
};

struct TopologySnapshot
{
    std::vector<DisplayRegion> Regions;

    bool IsValid() const { return !Regions.empty(); }

    void Reset() { Regions.clear(); }
};

} // namespace Transition