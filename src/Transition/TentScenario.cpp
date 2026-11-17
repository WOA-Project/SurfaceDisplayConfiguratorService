#include "pch.h"

#include "Transition\TentScenario.h"

#include <algorithm>
#include <cmath>

namespace Transition {

bool
TentScenarioBuilder::IsHorizontalArrangement(const VisualState &First, const VisualState &Second)
{
    const LONG DeltaX = std::abs(First.Rect.left - Second.Rect.left);

    const LONG DeltaY = std::abs(First.Rect.top - Second.Rect.top);

    return DeltaX > DeltaY;
}

HRESULT
TentScenarioBuilder::Build(Scenario &Scenario)
{
    if (Scenario.CurrentStates.size() != 2 || Scenario.TargetStates.size() != 2)
    {
        return E_INVALIDARG;
    }

    auto Left = Scenario.TargetStates[0];

    auto Right = Scenario.TargetStates[1];

    //
    // Never trust array ordering.
    //

    if (Left.Rect.left > Right.Rect.left)
    {
        std::swap(Left, Right);
    }

    if (!IsHorizontalArrangement(Left, Right))
    {
        return S_OK;
    }

    //
    // Hinge pivots.
    //

    Left.PivotX = static_cast<float>(Left.Rect.right);

    Left.PivotY = static_cast<float>(Left.Rect.top + ((Left.Rect.bottom - Left.Rect.top) / 2));

    Right.PivotX = static_cast<float>(Right.Rect.left);

    Right.PivotY = static_cast<float>(Right.Rect.top + ((Right.Rect.bottom - Right.Rect.top) / 2));

    //
    // Visual tent effect.
    //

    Left.RotationDegrees = -45.0f;

    Right.RotationDegrees = 45.0f;

    Scenario.TargetStates.clear();

    Scenario.TargetStates.push_back(Left);

    Scenario.TargetStates.push_back(Right);

    Scenario.DurationMs = 500;

    //
    // Topology update relatively early.
    //

    Scenario.CommitPoint = 0.35f;

    return S_OK;
}

} // namespace Transition
