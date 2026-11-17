#pragma once

#include "Transition\CompositionHost.h"

#include <dcomp.h>
#include <wrl/client.h>

namespace Transition {

class AnimationBuilder
{
public:
    explicit AnimationBuilder(CompositionHost &Host);

    AnimationBuilder(const AnimationBuilder &) = delete;

    AnimationBuilder &operator=(const AnimationBuilder &) = delete;

    //
    // Scalar animation helper.
    //
    // Creates a simple Start -> End animation over
    // DurationSeconds.
    //

    HRESULT CreateScalarAnimation(float Start, float End, float DurationSeconds, IDCompositionAnimation **Animation);

    //
    // Transform creation helpers.
    //

    HRESULT CreateTranslateTransform(IDCompositionTranslateTransform **Transform);

    HRESULT CreateScaleTransform(IDCompositionScaleTransform **Transform);

    HRESULT CreateRotateTransform(IDCompositionRotateTransform **Transform);

#if !TRANSITION_DISABLE_TRANSFORMGROUPS

    HRESULT
    CreateTransformGroup(IDCompositionTransform **Transforms, UINT TransformCount, IDCompositionTransform **Group);

#endif

    //
    // Convenience helpers.
    //

    HRESULT
    CreateOpacityAnimation(IDCompositionVisual *Visual, float StartOpacity, float EndOpacity, float DurationSeconds);

private:
    CompositionHost &m_Host;
};

} // namespace Transition