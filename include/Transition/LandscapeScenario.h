#pragma once

#include "Transition\TransitionScenarioBuilder.h"

namespace Transition {

class LandscapeScenarioBuilder
{
public:
    HRESULT Build(Scenario &Scenario);

private:
    static bool IsPortraitRect(const RECT &Rect) noexcept;

    static bool IsLandscapeRect(const RECT &Rect) noexcept;
};

} // namespace Transition