#pragma once

#include "Transition\TransitionScenarioBuilder.h"

namespace Transition {

class TentScenarioBuilder
{
public:
    HRESULT Build(Scenario &Scenario);

private:
    static bool IsHorizontalArrangement(const VisualState &First, const VisualState &Second);
};

} // namespace Transition