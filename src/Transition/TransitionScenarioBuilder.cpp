#include "pch.h"

#include "Transition\\TransitionScenarioBuilder.h"

#include "Transition\\BookScenario.h"
#include "Transition\\DualToSingleScenario.h"
#include "Transition\\PortraitScenario.h"
#include "Transition\\SingleToDualScenario.h"
#include "Transition\\TentScenario.h"

#include <algorithm>

namespace Transition {

namespace {

static float
ToDegrees(DisplayRotation Rotation)
{
    switch (Rotation)
    {
    case DisplayRotation::Rotate90:

        return 90.0f;

    case DisplayRotation::Rotate180:

        return 180.0f;

    case DisplayRotation::Rotate270:

        return 270.0f;

    default:

        return 0.0f;
    }
}

} // namespace

VisualState
TransitionScenarioBuilder::MakeState(const RenderGeometry &Geometry)
{
    VisualState state{};

    state.Id = Geometry.Id;

    state.Rect = Geometry.RenderRect;

    state.DesktopRect = Geometry.DesktopRect;

    state.TextureWidth = Geometry.TextureWidth;

    state.TextureHeight = Geometry.TextureHeight;

    state.RotationDegrees = Geometry.RotationDegrees;

    state.Opacity = 1.0f;

    state.PivotX = static_cast<float>(Geometry.TextureWidth) * 0.5f;

    state.PivotY = static_cast<float>(Geometry.TextureHeight) * 0.5f;

    return state;
}

HRESULT
TransitionScenarioBuilder::BuildScenario(
    const std::vector<RenderGeometry> &CurrentGeometry,
    const std::vector<RenderGeometry> &TargetGeometry,
    Scenario &Result)
{
    Result = {};

    Result.DurationMs = 250;

    Result.CommitPoint = 0.50f;

    const size_t currentCount = CurrentGeometry.size();

    const size_t targetCount = TargetGeometry.size();

    if (currentCount == 2 && targetCount == 1)
    {
        Result.Kind = ScenarioKind::DualToSingle;
    }
    else if (currentCount == 1 && targetCount == 2)
    {
        Result.Kind = ScenarioKind::SingleToDual;
    }
    else
    {
        Result.Kind = ScenarioKind::RotationChange;
    }

    for (const auto &geometry : CurrentGeometry)
    {
        Result.CurrentStates.emplace_back(MakeState(geometry));
    }

    for (const auto &geometry : TargetGeometry)
    {
        Result.TargetStates.emplace_back(MakeState(geometry));
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

    default: {
        PortraitScenarioBuilder builder;

        RETURN_IF_FAILED(builder.Build(Result));

        break;
    }
    }

    return S_OK;
}

} // namespace Transition