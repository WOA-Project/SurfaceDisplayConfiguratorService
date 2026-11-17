#include "pch.h"

#include "Transition\BookScenario.h"

#include <algorithm>
#include <cmath>

namespace Transition {

bool
BookScenarioBuilder::IsHorizontalArrangement(const VisualState &First, const VisualState &Second)
{
    const LONG DeltaX = std::abs(First.Rect.left - Second.Rect.left);

    const LONG DeltaY = std::abs(First.Rect.top - Second.Rect.top);

    return DeltaX > DeltaY;
}

HRESULT
BookScenarioBuilder::Build(Scenario &Scenario)
{
    if (Scenario.CurrentStates.size() != 2 || Scenario.TargetStates.size() != 2)
    {
        return E_INVALIDARG;
    }

    Scenario.DurationMs = 450;
    Scenario.CommitPoint = 0.50f;

    auto First = Scenario.CurrentStates[0];

    auto Second = Scenario.CurrentStates[1];

    //
    // Book mode is intended to transition
    // from a horizontal arrangement into
    // a vertically stacked arrangement.
    //

    if (!IsHorizontalArrangement(First, Second))
    {
        return S_OK;
    }

    //
    // Ensure deterministic ordering.
    //

    if (First.Rect.left > Second.Rect.left)
    {
        std::swap(First, Second);
    }

    const LONG Width = First.Rect.right - First.Rect.left;

    const LONG Height = First.Rect.bottom - First.Rect.top;

    VisualState TargetA = First;

    TargetA.Rect.left = 0;
    TargetA.Rect.top = 0;
    TargetA.Rect.right = Width;
    TargetA.Rect.bottom = Height;

    //
    // Rotate around the shared seam.
    //

    TargetA.PivotX = static_cast<float>(Width / 2);

    TargetA.PivotY = static_cast<float>(Height);

    VisualState TargetB = Second;

    TargetB.Rect.left = 0;
    TargetB.Rect.top = Height;
    TargetB.Rect.right = Width;
    TargetB.Rect.bottom = Height * 2;

    TargetB.PivotX = static_cast<float>(Width / 2);

    TargetB.PivotY = 0.0f;

    Scenario.TargetStates.clear();

    Scenario.TargetStates.push_back(TargetA);

    Scenario.TargetStates.push_back(TargetB);

    return S_OK;
}

} // namespace Transition
