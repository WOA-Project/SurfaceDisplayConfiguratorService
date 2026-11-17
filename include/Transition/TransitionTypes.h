#pragma once

#include <windows.h>

#include <cstdint>
#include <vector>

namespace Transition {

enum class Kind
{
    Unknown,

    DualToSingle,
    SingleToDual,

    LandscapeToPortrait,
    PortraitToLandscape,

    Fold,
    Unfold,

    Book,
    Tent
};

enum class AnimationFlags : uint32_t
{
    None = 0x0000,

    Opacity = 0x0001,

    Translation = 0x0002,

    Rotation = 0x0004,

    Scale = 0x0008,

    Default = Opacity | Translation | Rotation
};

inline AnimationFlags
operator|(AnimationFlags Left, AnimationFlags Right)
{
    return static_cast<AnimationFlags>(static_cast<uint32_t>(Left) | static_cast<uint32_t>(Right));
}

inline bool
HasFlag(AnimationFlags Value, AnimationFlags Flag)
{
    return (static_cast<uint32_t>(Value) & static_cast<uint32_t>(Flag)) != 0;
}

struct Region
{
    RECT CurrentRect{};

    RECT TargetRect{};

    float CurrentRotationDegrees = 0.0f;

    float TargetRotationDegrees = 0.0f;
};

struct Request
{
    Kind TransitionKind = Kind::Unknown;

    AnimationFlags Flags = AnimationFlags::Default;

    UINT DurationMs = 250;

    std::vector<Region> Regions;
};

} // namespace Transition