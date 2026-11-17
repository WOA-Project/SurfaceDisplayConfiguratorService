#include "pch.h"

#include "Transition\DisplayRegionMatcher.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Transition {

namespace {

LONG
Width(const RECT &Rect)
{
    return std::max(0L, Rect.right - Rect.left);
}

LONG
Height(const RECT &Rect)
{
    return std::max(0L, Rect.bottom - Rect.top);
}

LONG
Area(const RECT &Rect)
{
    return Width(Rect) * Height(Rect);
}

LONG
CenterX(const RECT &Rect)
{
    return (Rect.left + Rect.right) / 2;
}

LONG
CenterY(const RECT &Rect)
{
    return (Rect.top + Rect.bottom) / 2;
}

double
DistanceSquared(const RECT &A, const RECT &B)
{
    const double dx = static_cast<double>(CenterX(A) - CenterX(B));

    const double dy = static_cast<double>(CenterY(A) - CenterY(B));

    return (dx * dx) + (dy * dy);
}

LONG
IntersectionArea(const RECT &A, const RECT &B)
{
    RECT intersection{
        std::max(A.left, B.left), std::max(A.top, B.top), std::min(A.right, B.right), std::min(A.bottom, B.bottom)};

    return Area(intersection);
}

double
OverlapRatio(const RECT &A, const RECT &B)
{
    const LONG intersection = IntersectionArea(A, B);

    if (intersection <= 0)
    {
        return 0.0;
    }

    const LONG largestArea = std::max(Area(A), Area(B));

    if (largestArea <= 0)
    {
        return 0.0;
    }

    return static_cast<double>(intersection) / static_cast<double>(largestArea);
}

double
MatchScore(const RECT &Current, const RECT &Target)
{
    const double overlap = OverlapRatio(Current, Target);

    //
    // Strongly prefer overlap.
    //

    if (overlap > 0.0)
    {
        return overlap * 1000000.0;
    }

    //
    // Fallback to closest center.
    //

    return -DistanceSquared(Current, Target);
}

} // namespace

VisualState
DisplayRegionMatcher::MakeRemovedState(const VisualState &State)
{
    auto Removed = State;

    Removed.Opacity = 0.0f;

    return Removed;
}

VisualState
DisplayRegionMatcher::MakeAppearingState(const VisualState &State)
{
    auto Appearing = State;

    Appearing.Opacity = 0.0f;

    return Appearing;
}

HRESULT
DisplayRegionMatcher::Match(
    const std::vector<VisualState> &Current,
    const std::vector<VisualState> &Target,
    std::vector<RegionPair> &Pairs)
{
    Pairs.clear();

    std::vector<bool> targetUsed(Target.size(), false);

    //
    // Match every current region to the
    // best target region.
    //

    for (const auto &currentState : Current)
    {
        int bestIndex = -1;

        double bestScore = -std::numeric_limits<double>::max();

        for (size_t i = 0; i < Target.size(); ++i)
        {
            if (targetUsed[i])
            {
                continue;
            }

            const double score = MatchScore(currentState.Rect, Target[i].Rect);

            if (score > bestScore)
            {
                bestScore = score;
                bestIndex = static_cast<int>(i);
            }
        }

        RegionPair pair{};

        pair.Current = currentState;

        if (bestIndex >= 0)
        {
            pair.Target = Target[bestIndex];

            targetUsed[bestIndex] = true;
        }
        else
        {
            pair.Target = MakeRemovedState(currentState);
        }

        Pairs.emplace_back(std::move(pair));
    }

    //
    // Remaining target regions represent
    // displays appearing during the
    // transition.
    //

    for (size_t i = 0; i < Target.size(); ++i)
    {
        if (targetUsed[i])
        {
            continue;
        }

        RegionPair pair{};

        pair.Current = MakeAppearingState(Target[i]);

        pair.Target = Target[i];

        Pairs.emplace_back(std::move(pair));
    }

    return S_OK;
}

} // namespace Transition
