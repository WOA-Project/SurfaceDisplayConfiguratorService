#include "pch.h"

#include <InputStitching\InputSpaceManager.h>

#include <InputStitching\InputSpaceBuilder.h>
#include <InputStitching\InputSpacePublisher.h>
#include <InputStitching\InputSpaceValidation.h>
#include <InputStitching\Plan.h>

#include <InputStitching\InputSpaceDiagnostics.h>

namespace InputStitching {

namespace {

Plan g_CurrentPlan;

}

HRESULT
InputSpaceManager::Initialize()
{
    g_CurrentPlan.Reset();

    return S_OK;
}

HRESULT
InputSpaceManager::ApplyPolicy(const InputSpacePolicy &Policy)
{
    InputSpaceBuilder builder;

    Plan newPlan;

    HRESULT hr = builder.BuildPlan(Policy, &newPlan);

    if (FAILED(hr))
    {
        wprintf(
            L"g_InputSpaceManager.BuildPlan(Policy, &newPlan) Failed."
            L"failed: 0x%08X\n",
            hr);
        return hr;
    }

#ifdef _DEBUG
    OutputDebugStringW(DescribePlan(newPlan).c_str());
#endif

    wprintf(DescribePlan(newPlan).c_str());

    std::wstring reason;

    hr = ValidatePlan(newPlan, &reason);

    if (FAILED(hr))
    {
        newPlan.Debug.Status = hr;

        newPlan.Debug.ValidationPassed = false;

        newPlan.Debug.ValidationFailureReason = reason;

        return hr;
    }

    newPlan.Debug.Status = S_OK;

    newPlan.Debug.ValidationPassed = true;

    m_Policy = Policy;

    g_CurrentPlan = std::move(newPlan);

    return S_OK;
}

HRESULT
InputSpaceManager::Publish()
{
    if (!m_Policy.PublishToSystem)
    {
        g_CurrentPlan.Debug.Published = false;

        return S_FALSE;
    }

    InputSpacePublisher publisher;

    HRESULT hr = publisher.Publish(g_CurrentPlan);

    if (SUCCEEDED(hr))
    {
        g_CurrentPlan.Debug.Published = true;
    }

    return hr;
}

HRESULT
InputSpaceManager::Remove()
{
    InputSpacePublisher publisher;

    HRESULT hr = publisher.Remove(g_CurrentPlan.Space.Id);

    if (SUCCEEDED(hr))
    {
        g_CurrentPlan.Debug.Published = false;
    }

    return hr;
}

HRESULT
InputSpaceManager::Rebuild()
{
    return ApplyPolicy(m_Policy);
}

const Plan &
InputSpaceManager::CurrentPlan() const
{
    return g_CurrentPlan;
}

} // namespace InputStitching