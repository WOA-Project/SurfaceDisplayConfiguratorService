#include "pch.h"

#include "Transition\\AnimationPlanner.h"

namespace Transition {

float
AnimationPlanner::Width(const RECT &Rect) noexcept
{
    return static_cast<float>(Rect.right - Rect.left);
}

float
AnimationPlanner::Height(const RECT &Rect) noexcept
{
    return static_cast<float>(Rect.bottom - Rect.top);
}

float
AnimationPlanner::CenterX(const RECT &Rect) noexcept
{
    return static_cast<float>(Rect.left + ((Rect.right - Rect.left) / 2));
}

float
AnimationPlanner::CenterY(const RECT &Rect) noexcept
{
    return static_cast<float>(Rect.top + ((Rect.bottom - Rect.top) / 2));
}

HRESULT
AnimationPlanner::BuildPlan(const RegionPair &Transition, AnimationPlan &Plan)
{
    Plan = {};

    const RECT &Current = Transition.Current.Rect;

    const RECT &Target = Transition.Target.Rect;

    //
    // Translation
    //
    // Geometry is already normalized
    // into RenderRect before this stage.
    //

    Plan.StartX = static_cast<float>(Current.left);

    Plan.StartY = static_cast<float>(Current.top);

    Plan.EndX = static_cast<float>(Target.left);

    Plan.EndY = static_cast<float>(Target.top);

    //
    // Scale
    //

    const float CurrentWidth = Width(Current);

    const float CurrentHeight = Height(Current);

    const float TargetWidth = Width(Target);

    const float TargetHeight = Height(Target);

    Plan.StartScaleX = 1.0f;
    Plan.StartScaleY = 1.0f;

    Plan.EndScaleX = 1.0f;
    Plan.EndScaleY = 1.0f;

    if (CurrentWidth > 0.0f)
    {
        Plan.EndScaleX = TargetWidth / CurrentWidth;
    }

    if (CurrentHeight > 0.0f)
    {
        Plan.EndScaleY = TargetHeight / CurrentHeight;
    }

    //
    // Rotation
    //

    Plan.StartRotationDegrees = Transition.Current.RotationDegrees;

    Plan.EndRotationDegrees = Transition.Target.RotationDegrees;

    //
    // Opacity
    //

    Plan.StartOpacity = Transition.Current.Opacity;

    Plan.EndOpacity = Transition.Target.Opacity;

    //
    // Pivot
    //
    // Geometry model now uses
    // texture-local pivots.
    //

    Plan.PivotX = Transition.Current.PivotX;

    Plan.PivotY = Transition.Current.PivotY;

    if (Plan.PivotX == 0.0f && Plan.PivotY == 0.0f)
    {
        Plan.PivotX = static_cast<float>(Transition.Current.TextureWidth) * 0.5f;

        Plan.PivotY = static_cast<float>(Transition.Current.TextureHeight) * 0.5f;
    }

    //
    // Timing
    //

    Plan.DurationMs = 250;

    Plan.CommitPoint = 0.50f;

    return S_OK;
}

} // namespace Transition