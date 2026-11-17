#include "pch.h"

#include "Transition\PortraitScenario.h"

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

    Scenario.DurationMs = 400;
    Scenario.CommitPoint = 0.50f;

    const size_t count = std::min(Scenario.CurrentStates.size(), Scenario.TargetStates.size());

    for (size_t i = 0; i < count; ++i)
    {
        auto &current = Scenario.CurrentStates[i];

        auto &target = Scenario.TargetStates[i];

        if (IsLandscapeRect(current.Rect))
        {
            target.RotationDegrees = 90.0f;
        }

        //
        // Rotate around the center of the
        // current display region.
        //

        target.PivotX = static_cast<float>(current.Rect.left + ((current.Rect.right - current.Rect.left) / 2));

        target.PivotY = static_cast<float>(current.Rect.top + ((current.Rect.bottom - current.Rect.top) / 2));
    }

    return S_OK;
}

} // namespace Transition
