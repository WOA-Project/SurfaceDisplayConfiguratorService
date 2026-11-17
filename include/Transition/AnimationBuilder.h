#pragma once

#include "Transition\\CompositionHost.h"

#include <dcomp.h>

namespace Transition {

class AnimationBuilder
{
public:
    explicit AnimationBuilder(CompositionHost &Host);

    HRESULT CreateScalarAnimation(float Start, float End, float DurationSeconds, IDCompositionAnimation **Animation);

    HRESULT CreateTranslateTransform(IDCompositionTranslateTransform **Transform);

    HRESULT CreateScaleTransform(IDCompositionScaleTransform **Transform);

    HRESULT CreateRotateTransform(IDCompositionRotateTransform **Transform);

#if !TRANSITION_DISABLE_TRANSFORMGROUPS

    HRESULT
    CreateTransformGroup(IDCompositionTransform **Transforms, UINT TransformCount, IDCompositionTransform **Group);

#endif

    HRESULT
    CreateOpacityAnimation(IDCompositionVisual *Visual, float StartOpacity, float EndOpacity, float DurationSeconds);

private:
    CompositionHost &m_Host;
};

} // namespace Transition