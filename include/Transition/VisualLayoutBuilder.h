#pragma once

#include "Transition\\TopologySnapshot.h"

#include <vector>

namespace Transition {

struct VisualState
{
    std::wstring Id;

    DisplayRole Role = DisplayRole::Unknown;

    //
    // Geometry actually used by the
    // animation system.
    //
    RECT Rect{};

    //
    // Original desktop geometry.
    //
    RECT DesktopRect{};

    //
    // Actual rendered content size.
    //
    UINT TextureWidth = 0;

    UINT TextureHeight = 0;

    float RotationDegrees = 0.0f;

    float Opacity = 1.0f;

    //
    // Pivot in texture-local space.
    //
    float PivotX = 0.0f;

    float PivotY = 0.0f;
};

struct VisualTransition
{
    VisualState Current;

    VisualState Target;
};

class VisualLayoutBuilder
{
public:
    HRESULT BuildTransition(
        const std::vector<VisualState> &CurrentStates,
        const std::vector<VisualState> &TargetStates,
        std::vector<VisualTransition> &Result);
};

} // namespace Transition