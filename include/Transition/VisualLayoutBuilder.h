#pragma once

#include <windows.h>
#include "Transition\TopologySnapshot.h"

#include <vector>

namespace Transition {

struct VisualState
{
    std::wstring Id;

    DisplayRole Role = DisplayRole::Unknown;

    RECT Rect{};

    float RotationDegrees = 0.0f;

    float Opacity = 1.0f;

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