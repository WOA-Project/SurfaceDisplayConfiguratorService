#include "pch.h"

#include "Transition\LandscapeScenario.h"

#include <algorithm>

namespace Transition {

bool
LandscapeScenarioBuilder::IsPortraitRect(const RECT &Rect) noexcept
{
    return (Rect.bottom - Rect.top) > (Rect.right - Rect.left);
}

bool
LandscapeScenarioBuilder::IsLandscapeRect(const RECT &Rect) noexcept
{
    return (Rect.right - Rect.left) > (Rect.bottom - Rect.top);
}

HRESULT
LandscapeScenarioBuilder::Build(Scenario &Scenario)
{
    if (Scenario.CurrentStates.empty() || Scenario.TargetStates.empty())
    {
        return E_INVALIDARG;
    }

    //
    // Landscape transitions are generally
    // slightly faster than fold/unfold
    // but slower than a simple fade.
    //

    Scenario.DurationMs = 400;

    Scenario.CommitPoint = 0.50f;

    const size_t count = std::min(Scenario.CurrentStates.size(), Scenario.TargetStates.size());

    for (size_t i = 0; i < count; ++i)
    {
        auto &current = Scenario.CurrentStates[i];

        auto &target = Scenario.TargetStates[i];

        //
        // Portrait -> Landscape.
        //

        if (IsPortraitRect(current.Rect))
        {
            target.RotationDegrees = 0.0f;
        }

        //
        // Rotate around display center.
        //

        target.PivotX = static_cast<float>(current.Rect.left + ((current.Rect.right - current.Rect.left) / 2));

        target.PivotY = static_cast<float>(current.Rect.top + ((current.Rect.bottom - current.Rect.top) / 2));
    }

    return S_OK;
}

} // namespace Transition
