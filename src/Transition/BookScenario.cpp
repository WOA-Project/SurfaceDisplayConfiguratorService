#include "pch.h"

#include "Transition\\BookScenario.h"

namespace Transition {

HRESULT
BookScenarioBuilder::Build(Scenario &Scenario)
{
    Scenario.DurationMs = 400;

    Scenario.CommitPoint = 0.50f;

    for (auto &state : Scenario.CurrentStates)
    {
        state.RotationDegrees = 0.0f;
    }

    for (auto &state : Scenario.TargetStates)
    {
        state.RotationDegrees = 0.0f;
    }

    return S_OK;
}

} // namespace Transition