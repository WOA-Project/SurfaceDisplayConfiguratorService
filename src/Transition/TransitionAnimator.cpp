#include "pch.h"

#include "Transition\\TransitionAnimator.h"

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Transition {

TransitionAnimator::TransitionAnimator(CompositionHost &Host) : m_Host(Host), m_Builder(Host) {}

HRESULT
TransitionAnimator::Execute(PanelVisual &Visual, const AnimationPlan &Plan)
{
    RETURN_IF_FAILED(ApplyTranslation(Visual, Plan));

    RETURN_IF_FAILED(ApplyScale(Visual, Plan));

    RETURN_IF_FAILED(ApplyRotation(Visual, Plan));

    RETURN_IF_FAILED(ApplyOpacity(Visual, Plan));

    return S_OK;
}

HRESULT
TransitionAnimator::ApplyTranslation(PanelVisual &Visual, const AnimationPlan &Plan)
{
    ComPtr<IDCompositionAnimation> xAnimation;

    ComPtr<IDCompositionAnimation> yAnimation;

    const float seconds = static_cast<float>(Plan.DurationMs) / 1000.0f;

    RETURN_IF_FAILED(m_Builder.CreateScalarAnimation(Plan.StartX, Plan.EndX, seconds, xAnimation.GetAddressOf()));

    RETURN_IF_FAILED(m_Builder.CreateScalarAnimation(Plan.StartY, Plan.EndY, seconds, yAnimation.GetAddressOf()));

    RETURN_IF_FAILED(Visual.Translate->SetOffsetX(xAnimation.Get()));

    RETURN_IF_FAILED(Visual.Translate->SetOffsetY(yAnimation.Get()));

    return S_OK;
}

HRESULT
TransitionAnimator::ApplyScale(PanelVisual &Visual, const AnimationPlan &Plan)
{
    ComPtr<IDCompositionAnimation> xAnimation;

    ComPtr<IDCompositionAnimation> yAnimation;

    const float seconds = static_cast<float>(Plan.DurationMs) / 1000.0f;

    RETURN_IF_FAILED(Visual.Scale->SetCenterX(Plan.PivotX));

    RETURN_IF_FAILED(Visual.Scale->SetCenterY(Plan.PivotY));

    RETURN_IF_FAILED(
        m_Builder.CreateScalarAnimation(Plan.StartScaleX, Plan.EndScaleX, seconds, xAnimation.GetAddressOf()));

    RETURN_IF_FAILED(
        m_Builder.CreateScalarAnimation(Plan.StartScaleY, Plan.EndScaleY, seconds, yAnimation.GetAddressOf()));

    RETURN_IF_FAILED(Visual.Scale->SetScaleX(xAnimation.Get()));

    RETURN_IF_FAILED(Visual.Scale->SetScaleY(yAnimation.Get()));

    return S_OK;
}

HRESULT
TransitionAnimator::ApplyRotation(PanelVisual &Visual, const AnimationPlan &Plan)
{
    if (Plan.StartRotationDegrees == Plan.EndRotationDegrees)
    {
        return S_OK;
    }

    ComPtr<IDCompositionAnimation> animation;

    const float seconds = static_cast<float>(Plan.DurationMs) / 1000.0f;

    RETURN_IF_FAILED(Visual.Rotate->SetCenterX(Plan.PivotX));

    RETURN_IF_FAILED(Visual.Rotate->SetCenterY(Plan.PivotY));

    RETURN_IF_FAILED(m_Builder.CreateScalarAnimation(
        Plan.StartRotationDegrees, Plan.EndRotationDegrees, seconds, animation.GetAddressOf()));

    RETURN_IF_FAILED(Visual.Rotate->SetAngle(animation.Get()));

    return S_OK;
}

HRESULT
TransitionAnimator::ApplyOpacity(PanelVisual &, const AnimationPlan &)
{
    return S_OK;
}

HRESULT
TransitionAnimator::Commit()
{
    return m_Host.Commit();
}

} // namespace Transition