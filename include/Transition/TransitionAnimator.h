#pragma once

#include "Transition\AnimationBuilder.h"
#include "Transition\AnimationPlan.h"
#include "Transition\SurfaceFactory.h"

namespace Transition {

class TransitionAnimator
{
public:
    explicit TransitionAnimator(CompositionHost &Host);

    TransitionAnimator(const TransitionAnimator &) = delete;

    TransitionAnimator &operator=(const TransitionAnimator &) = delete;

    HRESULT Execute(PanelVisual &Visual, const AnimationPlan &Plan);

    HRESULT Commit();

private:
    HRESULT ApplyTranslation(PanelVisual &Visual, const AnimationPlan &Plan);

    HRESULT ApplyScale(PanelVisual &Visual, const AnimationPlan &Plan);

    HRESULT ApplyRotation(PanelVisual &Visual, const AnimationPlan &Plan);

    HRESULT ApplyOpacity(PanelVisual &Visual, const AnimationPlan &Plan);

private:
    CompositionHost &m_Host;

    AnimationBuilder m_Builder;
};

} // namespace Transition