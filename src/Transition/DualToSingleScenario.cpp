#include "pch.h"

#include "Transition\\DualToSingleScenario.h"

namespace Transition {

HRESULT
DualToSingleScenarioBuilder::Build(Scenario &Scenario)
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