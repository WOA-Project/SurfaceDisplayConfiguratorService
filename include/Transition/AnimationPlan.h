#pragma once

namespace Transition {

struct AnimationPlan
{
    //
    // Translation
    //

    float StartX = 0.0f;
    float StartY = 0.0f;

    float EndX = 0.0f;
    float EndY = 0.0f;

    //
    // Scale
    //

    float StartScaleX = 1.0f;
    float StartScaleY = 1.0f;

    float EndScaleX = 1.0f;
    float EndScaleY = 1.0f;

    //
    // Rotation
    //

    float StartRotationDegrees = 0.0f;
    float EndRotationDegrees = 0.0f;

    //
    // Opacity
    //

    float StartOpacity = 1.0f;
    float EndOpacity = 1.0f;

    //
    // Pivot
    //

    float PivotX = 0.0f;
    float PivotY = 0.0f;

    //
    // Timing
    //

    UINT DurationMs = 250;

    float CommitPoint = 0.50f;
};

} // namespace Transition