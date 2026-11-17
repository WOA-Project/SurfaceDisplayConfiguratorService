#include "pch.h"

#include "Transition\SingleToDualScenario.h"

#include <algorithm>
#include <cmath>

namespace Transition {

namespace {

POINT
Center(const RECT &Rect)
{
    POINT pt{};

    pt.x = (Rect.left + Rect.right) / 2;

    pt.y = (Rect.top + Rect.bottom) / 2;

    return pt;
}

LONG
DistanceSquared(const POINT &A, const POINT &B)
{
    const LONG dx = A.x - B.x;

    const LONG dy = A.y - B.y;

    return (dx * dx) + (dy * dy);
}

} // namespace

HRESULT
SingleToDualScenarioBuilder::Build(Scenario &Scenario)
{
    if (Scenario.CurrentStates.size() != 1 || Scenario.TargetStates.size() != 2)
    {
        return E_INVALIDARG;
    }

    auto Expanded = Scenario.CurrentStates[0];

    auto TargetA = Scenario.TargetStates[0];

    auto TargetB = Scenario.TargetStates[1];

    const auto CurrentCenter = Center(Expanded.Rect);

    const auto CenterA = Center(TargetA.Rect);

    const auto CenterB = Center(TargetB.Rect);

    const LONG DistanceA = DistanceSquared(CurrentCenter, CenterA);

    const LONG DistanceB = DistanceSquared(CurrentCenter, CenterB);

    VisualState Existing;
    VisualState Appearing;

    if (DistanceA <= DistanceB)
    {
        Existing = TargetA;

        Appearing = TargetB;
    }
    else
    {
        Existing = TargetB;

        Appearing = TargetA;
    }

    //
    // New display starts at the hinge and
    // fades/expands into existence.
    //

    auto InitialAppearing = Appearing;

    InitialAppearing.Opacity = 0.0f;

    InitialAppearing.Rect = Existing.Rect;

    //
    // Hinge pivot.
    //

    Existing.PivotX = static_cast<float>(Existing.Rect.right);

    Existing.PivotY = static_cast<float>(Existing.Rect.top + ((Existing.Rect.bottom - Existing.Rect.top) / 2));

    Appearing.PivotX = Existing.PivotX;

    Appearing.PivotY = Existing.PivotY;

    //
    // Normalize cardinality.
    //

    Scenario.CurrentStates.clear();

    Scenario.CurrentStates.push_back(Expanded);

    Scenario.CurrentStates.push_back(InitialAppearing);

    Scenario.TargetStates.clear();

    Scenario.TargetStates.push_back(Existing);

    Scenario.TargetStates.push_back(Appearing);

    Scenario.DurationMs = 350;

    //
    // Topology becomes available early.
    //

    Scenario.CommitPoint = 0.30f;

    return S_OK;
}

} // namespace Transition
