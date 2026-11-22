#include "pch.h"

#include <windows.h>

#include <InputStitching\InputSpaceDiagnostics.h>
#include <InputStitching\InputSpaceGlobals.h>
#include <InputStitching\InputSpaceManager.h>
#include <InputStitching\InputSpacePolicy.h>

#define OutputDebugStringW wprintf

namespace InputStitching {

namespace {

void
PrintCurrentPlan()
{
    const auto &plan = g_InputSpaceManager.CurrentPlan();

    const auto dump = DescribePlan(plan);

    OutputDebugStringW(L"\r\n");

    OutputDebugStringW(dump.c_str());

    OutputDebugStringW(L"\r\n");
}

HRESULT
ApplyAndDump(const InputSpacePolicy &Policy)
{
    HRESULT hr = g_InputSpaceManager.ApplyPolicy(Policy);

    if (FAILED(hr))
    {
        return hr;
    }

    PrintCurrentPlan();

    return S_OK;
}

} // namespace

HRESULT
TestDualIndependent()
{
    InputSpacePolicy policy{};

    policy.Mode = InputTopologyMode::DualIndependent;

    policy.EnableStitching = false;

    policy.EnableGapRegion = false;

    return ApplyAndDump(policy);
}

HRESULT
TestDualStitched()
{
    InputSpacePolicy policy{};

    policy.Mode = InputTopologyMode::DualStitched;

    policy.EnableStitching = true;

    policy.EnableGapRegion = true;

    //
    // Surface Duo hinge width.
    // Adjust during testing as needed.
    //
    policy.GapPixels = 66;

    return ApplyAndDump(policy);
}

HRESULT
TestSingleLeft()
{
    InputSpacePolicy policy{};

    policy.Mode = InputTopologyMode::SingleLeft;

    return ApplyAndDump(policy);
}

HRESULT
TestSingleRight()
{
    InputSpacePolicy policy{};

    policy.Mode = InputTopologyMode::SingleRight;

    return ApplyAndDump(policy);
}

HRESULT
TestPublish()
{
    HRESULT hr = TestDualStitched();

    if (FAILED(hr))
    {
        return hr;
    }

    hr = g_InputSpaceManager.Publish();

    const auto &plan = g_InputSpaceManager.CurrentPlan();

    auto report = DescribePlan(plan);

    OutputDebugStringW(report.c_str());

    return hr;
}

HRESULT
TestRemove()
{
    return g_InputSpaceManager.Remove();
}

HRESULT
DumpCurrentPlan()
{
    PrintCurrentPlan();

    return S_OK;
}

HRESULT
RunInputSpaceTests()
{
    HRESULT hr;

    wprintf(L"TestDualIndependent\n");

    hr = TestDualIndependent();

    if (FAILED(hr))
    {
        wprintf(
            L"FAILED TestDualIndependent "
            L"0x%08X\n",
            hr);

        return hr;
    }

    wprintf(L"TestDualStitched\n");

    hr = TestDualStitched();

    if (FAILED(hr))
    {
        wprintf(
            L"FAILED TestDualStitched "
            L"0x%08X\n",
            hr);

        return hr;
    }

    wprintf(L"TestSingleLeft\n");

    hr = TestSingleLeft();

    if (FAILED(hr))
    {
        wprintf(
            L"FAILED TestSingleLeft "
            L"0x%08X\n",
            hr);

        return hr;
    }

    wprintf(L"TestSingleRight\n");

    hr = TestSingleRight();

    if (FAILED(hr))
    {
        wprintf(
            L"FAILED TestSingleRight "
            L"0x%08X\n",
            hr);

        return hr;
    }

    return S_OK;
}

} // namespace InputStitching