#include "pch.h"

#include "Transition/TransitionScenarioBuilder.h"

#include "Transition/BookScenario.h"
#include "Transition/DualToSingleScenario.h"
#include "Transition/PortraitScenario.h"
#include "Transition/SingleToDualScenario.h"
#include "Transition/TentScenario.h"
#include "Transition/TopologySnapshot.h"

#include <algorithm>
#include <cmath>

namespace Transition {

namespace {

const DisplayRegion *
FindRegionByRole(const TopologySnapshot &Snapshot, DisplayRole Role)
{
    for (const auto &region : Snapshot.Regions)
    {
        if (region.Role == Role)
        {
            return &region;
        }
    }

    return nullptr;
}

} // namespace

VisualState
TransitionScenarioBuilder::MakeState(const DisplayRegion &Region)
{
    VisualState state{};

    state.Id = Region.Id;

    state.Role = Region.Role;

    //
    // Use RenderRect as the geometry
    // being animated.
    //
    state.Rect = Region.RenderRect;

    //
    // Preserve desktop placement
    // separately.
    //
    state.DesktopRect = Region.DesktopRect;

    state.TextureWidth = Region.TextureWidth;

    state.TextureHeight = Region.TextureHeight;

    state.RotationDegrees = 0.0f;

    state.Opacity = 1.0f;

    //
    // Pivot is texture-local.
    //
    state.PivotX = static_cast<float>(Region.TextureWidth) * 0.5f;

    state.PivotY = static_cast<float>(Region.TextureHeight) * 0.5f;

    return state;
}

ScenarioKind
TransitionScenarioBuilder::DetectScenario(const TopologySnapshot &Current, const TopologySnapshot &Target)
{
    const size_t currentCount = Current.Regions.size();

    const size_t targetCount = Target.Regions.size();

    if (currentCount == 2 && targetCount == 1)
    {
        return ScenarioKind::DualToSingle;
    }

    if (currentCount == 1 && targetCount == 2)
    {
        return ScenarioKind::SingleToDual;
    }

    const DisplayRegion *currentLeft = FindRegionByRole(Current, DisplayRole::Left);

    const DisplayRegion *targetLeft = FindRegionByRole(Target, DisplayRole::Left);

    if (currentLeft && targetLeft)
    {
        if (currentLeft->Rotation != targetLeft->Rotation)
        {
            return ScenarioKind::RotationChange;
        }
    }

    if (targetCount == 2)
    {
        const auto &first = Target.Regions[0];

        const auto &second = Target.Regions[1];

        const LONG deltaX = std::abs(first.DesktopRect.left - second.DesktopRect.left);

        const LONG deltaY = std::abs(first.DesktopRect.top - second.DesktopRect.top);

        if (deltaY > deltaX)
        {
            return ScenarioKind::Book;
        }
    }

    return ScenarioKind::Unknown;
}

HRESULT
TransitionScenarioBuilder::BuildScenario(
    const TopologySnapshot &CurrentSnapshot,
    const TopologySnapshot &TargetSnapshot,
    Scenario &Result)
{
    Result = {};

    Result.Kind = DetectScenario(CurrentSnapshot, TargetSnapshot);

    Result.DurationMs = 250;

    Result.CommitPoint = 0.50f;

    for (const auto &region : CurrentSnapshot.Regions)
    {
        Result.CurrentStates.push_back(MakeState(region));
    }

    for (const auto &region : TargetSnapshot.Regions)
    {
        Result.TargetStates.push_back(MakeState(region));
    }

    switch (Result.Kind)
    {
    case ScenarioKind::DualToSingle: {
        DualToSingleScenarioBuilder builder;

        RETURN_IF_FAILED(builder.Build(Result));

        break;
    }

    case ScenarioKind::SingleToDual: {
        SingleToDualScenarioBuilder builder;

        RETURN_IF_FAILED(builder.Build(Result));

        break;
    }

    case ScenarioKind::Book: {
        BookScenarioBuilder builder;

        RETURN_IF_FAILED(builder.Build(Result));

        break;
    }

    case ScenarioKind::Tent: {
        TentScenarioBuilder builder;

        RETURN_IF_FAILED(builder.Build(Result));

        break;
    }

    case ScenarioKind::RotationChange:
    case ScenarioKind::LandscapeToPortrait:
    case ScenarioKind::PortraitToLandscape: {
        PortraitScenarioBuilder builder;

        RETURN_IF_FAILED(builder.Build(Result));

        break;
    }

    default:

        break;
    }

    return S_OK;
}

} // namespace Transition