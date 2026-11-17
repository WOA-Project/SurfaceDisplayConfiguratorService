#include "pch.h"

#include "Transition\\PortraitScenario.h"

#include <algorithm>

namespace Transition {

bool
PortraitScenarioBuilder::IsPortraitRect(const RECT &Rect) noexcept
{
    return (Rect.bottom - Rect.top) > (Rect.right - Rect.left);
}

bool
PortraitScenarioBuilder::IsLandscapeRect(const RECT &Rect) noexcept
{
    return (Rect.right - Rect.left) > (Rect.bottom - Rect.top);
}

HRESULT
PortraitScenarioBuilder::Build(Scenario &Scenario)
{
    if (Scenario.CurrentStates.empty() || Scenario.TargetStates.empty())
    {
        return E_INVALIDARG;
    }

    //
    // Portrait transitions are largely
    // geometry-driven now.
    //
    // RotationDegrees already comes from:
    //
    // TopologySnapshot
    //        ↓
    // MakeState()
    //
    // Therefore we do NOT overwrite
    // rotation values here.
    //

    Scenario.DurationMs = 400;

    Scenario.CommitPoint = 0.50f;

    const size_t count = std::min(Scenario.CurrentStates.size(), Scenario.TargetStates.size());

    for (size_t i = 0; i < count; ++i)
    {
        auto &current = Scenario.CurrentStates[i];

        auto &target = Scenario.TargetStates[i];

        //
        // Rotate around display center.
        //

        //target.PivotX = static_cast<float>(current.Rect.left + ((current.Rect.right - current.Rect.left) / 2));

        //target.PivotY = static_cast<float>(current.Rect.top + ((current.Rect.bottom - current.Rect.top) / 2));

        target.PivotX = 675.0f;
target.PivotY = 900.0f;
    }

    //
    // Diagnostics.
    //

    for (size_t i = 0; i < count; ++i)
    {
        wprintf(
            L"PORTRAIT SCENARIO[%zu] "
            L"%f -> %f\n",
            i,
            Scenario.CurrentStates[i].RotationDegrees,
            Scenario.TargetStates[i].RotationDegrees);
    }

    return S_OK;
}

} // namespace Transition