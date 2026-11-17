#include "pch.h"

#include "Transition\DualToSingleScenario.h"

#include <algorithm>

namespace Transition {

namespace {

LONG
IntersectionArea(const RECT &A, const RECT &B)
{
    const LONG left = std::max(A.left, B.left);

    const LONG top = std::max(A.top, B.top);

    const LONG right = std::min(A.right, B.right);

    const LONG bottom = std::min(A.bottom, B.bottom);

    const LONG width = std::max(0L, right - left);

    const LONG height = std::max(0L, bottom - top);

    return width * height;
}

LONG
Area(const RECT &Rect)
{
    return std::max(0L, Rect.right - Rect.left) * std::max(0L, Rect.bottom - Rect.top);
}

double
OverlapRatio(const RECT &A, const RECT &B)
{
    const LONG intersection = IntersectionArea(A, B);

    const LONG largest = std::max(Area(A), Area(B));

    if (largest <= 0)
    {
        return 0.0;
    }

    return static_cast<double>(intersection) / static_cast<double>(largest);
}

} // namespace

HRESULT
DualToSingleScenarioBuilder::Build(Scenario &Scenario)
{
    if (Scenario.CurrentStates.size() != 2 || Scenario.TargetStates.size() != 1)
    {
        return E_INVALIDARG;
    }

    auto CurrentA = Scenario.CurrentStates[0];

    auto CurrentB = Scenario.CurrentStates[1];

    auto Target = Scenario.TargetStates[0];

    const double overlapA = OverlapRatio(CurrentA.Rect, Target.Rect);

    const double overlapB = OverlapRatio(CurrentB.Rect, Target.Rect);

    VisualState Surviving;
    VisualState Removed;

    if (overlapA >= overlapB)
    {
        Surviving = CurrentA;

        Removed = CurrentB;
    }
    else
    {
        Surviving = CurrentB;

        Removed = CurrentA;
    }

    VisualState Expanded = Surviving;

    Expanded.Rect = Target.Rect;

    Expanded.Opacity = 1.0f;

    Expanded.PivotX = static_cast<float>(Surviving.Rect.right);

    Expanded.PivotY = static_cast<float>(Surviving.Rect.top + ((Surviving.Rect.bottom - Surviving.Rect.top) / 2));

    Removed.Opacity = 0.0f;

    Scenario.TargetStates.clear();

    Scenario.TargetStates.push_back(Expanded);

    Scenario.TargetStates.push_back(Removed);

    Scenario.DurationMs = 350;

    Scenario.CommitPoint = 0.70f;

    return S_OK;
}

} // namespace Transition
