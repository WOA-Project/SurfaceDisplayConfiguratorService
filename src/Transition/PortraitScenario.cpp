#include "pch.h"

#include "Transition\\PortraitScenario.h"

#include <algorithm>

namespace Transition {

static float
NormalizeDegrees(float Degrees)
{
    while (Degrees < 0.0f)
    {
        Degrees += 360.0f;
    }

    while (Degrees >= 360.0f)
    {
        Degrees -= 360.0f;
    }

    return Degrees;
}

static float
ShortestArcTarget(float Start, float End)
{
    Start = NormalizeDegrees(Start);

    End = NormalizeDegrees(End);

    float Delta = End - Start;

    if (Delta > 180.0f)
    {
        Delta -= 360.0f;
    }

    if (Delta < -180.0f)
    {
        Delta += 360.0f;
    }

    return Start + Delta;
}

HRESULT
PortraitScenarioBuilder::Build(Scenario &Scenario)
{
    Scenario.DurationMs = 400;

    Scenario.CommitPoint = 0.50f;

    const size_t count = std::min(Scenario.CurrentStates.size(), Scenario.TargetStates.size());

    for (size_t i = 0; i < count; ++i)
    {
        auto &current = Scenario.CurrentStates[i];

        auto &target = Scenario.TargetStates[i];

        target.RotationDegrees = ShortestArcTarget(current.RotationDegrees, target.RotationDegrees);
    }

    return S_OK;
}

} // namespace Transition