#include "pch.h"

#include "Transition\\TransitionAnimator.h"

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace Transition
{

TransitionAnimator::TransitionAnimator(
    CompositionHost& Host)
    :
    m_Host(Host),
    m_Builder(Host)
{
}

HRESULT
TransitionAnimator::Execute(
    PanelVisual& Visual,
    const AnimationPlan& Plan)
{
    RETURN_IF_FAILED(
        ApplyTranslation(
            Visual,
            Plan));

    RETURN_IF_FAILED(
        ApplyScale(
            Visual,
            Plan));

    RETURN_IF_FAILED(
        ApplyRotation(
            Visual,
            Plan));

    RETURN_IF_FAILED(
        ApplyOpacity(
            Visual,
            Plan));

    return S_OK;
}

HRESULT
TransitionAnimator::ApplyTranslation(
    PanelVisual& Visual,
    const AnimationPlan& Plan)
{
    wprintf(
        L"[Transition] APPLY TRANSLATION "
        L"(%f,%f)->(%f,%f)\n",
        Plan.StartX,
        Plan.StartY,
        Plan.EndX,
        Plan.EndY);

    ComPtr<IDCompositionAnimation>
        xAnimation;

    ComPtr<IDCompositionAnimation>
        yAnimation;

    const float seconds =
        static_cast<float>(
            Plan.DurationMs) /
        1000.0f;

    RETURN_IF_FAILED(
        m_Builder.CreateScalarAnimation(
            Plan.StartX,
            Plan.EndX,
            seconds,
            xAnimation.GetAddressOf()));

    RETURN_IF_FAILED(
        m_Builder.CreateScalarAnimation(
            Plan.StartY,
            Plan.EndY,
            seconds,
            yAnimation.GetAddressOf()));

    HRESULT hr;

    hr =
        Visual.Translate->SetOffsetX(
            xAnimation.Get());

    wprintf(
        L"[Transition] SetOffsetX(animation) = 0x%08X\n",
        hr);

    RETURN_IF_FAILED(
        hr);

    hr =
        Visual.Translate->SetOffsetY(
            yAnimation.Get());

    wprintf(
        L"[Transition] SetOffsetY(animation) = 0x%08X\n",
        hr);

    RETURN_IF_FAILED(
        hr);

    return S_OK;
}

HRESULT
TransitionAnimator::ApplyScale(
    PanelVisual& Visual,
    const AnimationPlan& Plan)
{
    ComPtr<IDCompositionAnimation>
        xAnimation;

    ComPtr<IDCompositionAnimation>
        yAnimation;

    const float seconds =
        static_cast<float>(
            Plan.DurationMs) /
        1000.0f;

    RETURN_IF_FAILED(
        Visual.Scale->SetCenterX(
            Plan.PivotX));

    RETURN_IF_FAILED(
        Visual.Scale->SetCenterY(
            Plan.PivotY));

    RETURN_IF_FAILED(
        m_Builder.CreateScalarAnimation(
            Plan.StartScaleX,
            Plan.EndScaleX,
            seconds,
            xAnimation.GetAddressOf()));

    RETURN_IF_FAILED(
        m_Builder.CreateScalarAnimation(
            Plan.StartScaleY,
            Plan.EndScaleY,
            seconds,
            yAnimation.GetAddressOf()));

    HRESULT hr;

    hr =
        Visual.Scale->SetScaleX(
            xAnimation.Get());

    wprintf(
        L"[Transition] SetScaleX(animation) = 0x%08X\n",
        hr);

    RETURN_IF_FAILED(
        hr);

    hr =
        Visual.Scale->SetScaleY(
            yAnimation.Get());

    wprintf(
        L"[Transition] SetScaleY(animation) = 0x%08X\n",
        hr);

    RETURN_IF_FAILED(
        hr);

    return S_OK;
}

HRESULT
TransitionAnimator::ApplyRotation(
    PanelVisual& Visual,
    const AnimationPlan& Plan)
{
    wprintf(
        L"[Transition] APPLY ROTATION "
        L"(%f)->(%f)\n",
        Plan.StartRotationDegrees,
        Plan.EndRotationDegrees);

    ComPtr<IDCompositionAnimation>
        animation;

    const float seconds =
        static_cast<float>(
            Plan.DurationMs) /
        1000.0f;

    RETURN_IF_FAILED(
        Visual.Rotate->SetCenterX(
            static_cast<float>(
        Visual.Width) / 2.0f));

    RETURN_IF_FAILED(
        Visual.Rotate->SetCenterY(
            static_cast<float>(
        Visual.Height) / 2.0f));

    RETURN_IF_FAILED(
        m_Builder.CreateScalarAnimation(
            Plan.StartRotationDegrees,
            Plan.EndRotationDegrees,
            seconds,
            animation.GetAddressOf()));

    HRESULT hr =
        Visual.Rotate->SetAngle(
            animation.Get());

    wprintf(
        L"[Transition] SetAngle(animation) = 0x%08X\n",
        hr);

    RETURN_IF_FAILED(
        hr);

    return S_OK;
}

HRESULT
TransitionAnimator::ApplyOpacity(
    PanelVisual&,
    const AnimationPlan&)
{
    //
    // Opacity disabled.
    //

    return S_OK;
}

HRESULT
TransitionAnimator::Commit()
{
    return m_Host.Commit();
}

} // namespace Transition