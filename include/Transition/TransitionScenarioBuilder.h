#pragma once

#include "Transition\\RenderGeometry.h"
#include "Transition\\TransitionScenario.h"

#include <vector>

namespace Transition {

class TransitionScenarioBuilder
{
public:
    HRESULT BuildScenario(
        const std::vector<RenderGeometry> &CurrentGeometry,
        const std::vector<RenderGeometry> &TargetGeometry,
        Scenario &Result);

private:
    VisualState MakeState(const RenderGeometry &Geometry);
};

} // namespace Transition