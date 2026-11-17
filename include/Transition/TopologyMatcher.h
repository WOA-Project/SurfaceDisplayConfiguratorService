#pragma once

#include "Transition\\RegionPair.h"
#include "Transition\\TopologySnapshot.h"

#include <vector>

namespace Transition {

class TopologyMatcher
{
public:
    HRESULT Match(const TopologySnapshot &Current, const TopologySnapshot &Target, std::vector<RegionPair> &Result);
};

} // namespace Transition