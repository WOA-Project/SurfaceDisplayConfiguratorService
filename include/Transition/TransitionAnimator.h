#pragma once

#include "Transition\\AnimationBuilder.h"
#include "Transition\\AnimationPlan.h"
#include "Transition\\CompositionHost.h"
#include "Transition\\PanelVisual.h"

namespace Transition {

class TransitionAnimator
{
public:
    explicit TransitionAnimator(CompositionHost &Host);

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