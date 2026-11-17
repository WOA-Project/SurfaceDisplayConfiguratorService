#pragma once

#include "Transition\\AnimationPlan.h"
#include "Transition\\RegionPair.h"

namespace Transition {

class AnimationPlanner
{
public:
    HRESULT BuildPlan(const RegionPair &Transition, AnimationPlan &Plan);

private:
    static float Width(const RECT &Rect) noexcept;

    static float Height(const RECT &Rect) noexcept;

    static float CenterX(const RECT &Rect) noexcept;

    static float CenterY(const RECT &Rect) noexcept;
};

} // namespace Transition