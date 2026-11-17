#pragma once

#include "Transition\VisualLayoutBuilder.h"
#include "Transition\TopologySnapshot.h"
#include "Transition\TransitionScenario.h"

namespace Transition {

class TransitionScenarioBuilder
{
public:
    HRESULT
    BuildScenario(const TopologySnapshot &CurrentPolicy, const TopologySnapshot &TargetPolicy, Scenario &Result);

private:
    static VisualState MakeState(const DisplayRegion &Region);

    static ScenarioKind DetectScenario(const TopologySnapshot &Current, const TopologySnapshot &Target);
};

} // namespace Transition