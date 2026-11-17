#include "pch.h"

#include "Transition\AnimationBuilder.h"

using Microsoft::WRL::ComPtr;

namespace Transition {

AnimationBuilder::AnimationBuilder(CompositionHost &Host) : m_Host(Host) {}

HRESULT
AnimationBuilder::CreateScalarAnimation(
    float Start,
    float End,
    float DurationSeconds,
    IDCompositionAnimation **Animation)
{
    if (!Animation)
    {
        return E_INVALIDARG;
    }

    *Animation = nullptr;

    auto *device = m_Host.GetCompositionDevice();

    if (!device)
    {
        return E_UNEXPECTED;
    }

    RETURN_IF_FAILED(device->CreateAnimation(Animation));

    RETURN_IF_FAILED((*Animation)->AddCubic(0.0, Start, 0.0, 0.0, 0.0));

    RETURN_IF_FAILED((*Animation)->AddCubic(DurationSeconds, End, 0.0, 0.0, 0.0));

    return S_OK;
}

HRESULT
AnimationBuilder::CreateTranslateTransform(IDCompositionTranslateTransform **Transform)
{
    if (!Transform)
    {
        return E_INVALIDARG;
    }

    *Transform = nullptr;

    auto *device = m_Host.GetCompositionDevice();

    if (!device)
    {
        return E_UNEXPECTED;
    }

    return device->CreateTranslateTransform(Transform);
}

HRESULT
AnimationBuilder::CreateScaleTransform(IDCompositionScaleTransform **Transform)
{
    if (!Transform)
    {
        return E_INVALIDARG;
    }

    *Transform = nullptr;

    auto *device = m_Host.GetCompositionDevice();

    if (!device)
    {
        return E_UNEXPECTED;
    }

    return device->CreateScaleTransform(Transform);
}

HRESULT
AnimationBuilder::CreateRotateTransform(IDCompositionRotateTransform **Transform)
{
    if (!Transform)
    {
        return E_INVALIDARG;
    }

    *Transform = nullptr;

    auto *device = m_Host.GetCompositionDevice();

    if (!device)
    {
        return E_UNEXPECTED;
    }

    return device->CreateRotateTransform(Transform);
}

#if !TRANSITION_DISABLE_TRANSFORMGROUPS

HRESULT
AnimationBuilder::CreateTransformGroup(
    IDCompositionTransform **Transforms,
    UINT TransformCount,
    IDCompositionTransform **Group)
{
    if (!Transforms || !Group)
    {
        return E_INVALIDARG;
    }

    *Group = nullptr;

    auto *device = m_Host.GetCompositionDevice();

    if (!device)
    {
        return E_UNEXPECTED;
    }

    return device->CreateTransformGroup(Transforms, TransformCount, Group);
}

#endif

/*HRESULT
AnimationBuilder::CreateOpacityAnimation(
    IDCompositionVisual* Visual,
    float StartOpacity,
    float EndOpacity,
    float DurationSeconds)
{
    if (!Visual)
    {
        return E_INVALIDARG;
    }

    ComPtr<IDCompositionAnimation>
        animation;

    RETURN_IF_FAILED(
        CreateScalarAnimation(
            StartOpacity,
            EndOpacity,
            DurationSeconds,
            animation.GetAddressOf()));

#ifndef TRANSITION_DISABLE_OPACITY
    return Visual->SetOpacity(
        animation.Get());
#else
    return S_OK;
#endif
}*/

HRESULT
AnimationBuilder::CreateOpacityAnimation(
    IDCompositionVisual *Visual,
    float StartOpacity,
    float EndOpacity,
    float DurationSeconds)
{
    UNREFERENCED_PARAMETER(StartOpacity);
    UNREFERENCED_PARAMETER(EndOpacity);
    UNREFERENCED_PARAMETER(DurationSeconds);

    return S_OK;
}

} // namespace Transition
