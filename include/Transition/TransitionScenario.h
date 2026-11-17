#pragma once

#include "Transition\\VisualLayoutBuilder.h"

#include <cstdint>
#include <vector>

namespace Transition
{

enum class ScenarioKind
{
    Unknown,

    LandscapeToPortrait,
    PortraitToLandscape,

    RotationChange,

    DualToSingle,
    SingleToDual,

    Book,
    Tent
};

struct Scenario
{
    ScenarioKind Kind =
        ScenarioKind::Unknown;

    std::vector<VisualState>
        CurrentStates;

    std::vector<VisualState>
        TargetStates;

    uint32_t DurationMs =
        250;

    float CommitPoint =
        0.50f;
};

} // namespace Transition