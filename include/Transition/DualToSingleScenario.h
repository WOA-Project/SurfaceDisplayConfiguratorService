#pragma once

#include "Transition\TransitionScenarioBuilder.h"

namespace Transition {

class DualToSingleScenarioBuilder
{
public:
    HRESULT Build(Scenario &Scenario);
};

} // namespace Transition