#pragma once

#include "Transition\VisualLayoutBuilder.h"
#include "Transition\TopologySnapshot.h"

namespace Transition {

enum class ScenarioKind
{
    Unknown,

    RotationChange,

    Fold,
    Unfold,

    SingleToDual,
    DualToSingle,

    LandscapeToPortrait,
    PortraitToLandscape,

    Book,
    Tent
};

struct Scenario
{
    ScenarioKind Kind = ScenarioKind::Unknown;

    std::vector<VisualState> CurrentStates;

    std::vector<VisualState> TargetStates;

    UINT DurationMs = 250;

    float CommitPoint = 0.50f;
};

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