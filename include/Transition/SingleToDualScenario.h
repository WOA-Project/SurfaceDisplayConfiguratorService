#pragma once

#include "Transition\TransitionScenarioBuilder.h"

namespace Transition {

class SingleToDualScenarioBuilder
{
public:
    HRESULT Build(Scenario &Scenario);
};

} // namespace Transition
