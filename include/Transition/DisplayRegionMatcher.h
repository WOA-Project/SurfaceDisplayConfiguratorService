#pragma once

#include "Transition\VisualLayoutBuilder.h"

#include <vector>

namespace Transition {

struct RegionPair
{
    VisualState Current;

    VisualState Target;
};

class DisplayRegionMatcher
{
public:
    HRESULT Match(
        const std::vector<VisualState> &Current,
        const std::vector<VisualState> &Target,
        std::vector<RegionPair> &Pairs);

private:
    static VisualState MakeRemovedState(const VisualState &State);

    static VisualState MakeAppearingState(const VisualState &State);
};

} // namespace Transition