#pragma once

#include "Transition\\RegionPair.h"
#include "Transition\\TransitionScenario.h"

#include <vector>

namespace Transition {

class DisplayRegionMatcher
{
public:
    HRESULT Match(
        const std::vector<VisualState> &CurrentStates,
        const std::vector<VisualState> &TargetStates,
        std::vector<RegionPair> &Result);
};

} // namespace Transition