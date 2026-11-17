#include "pch.h"

#include "Transition/TopologyMatcher.h"

namespace Transition {

const DisplayRegion *
FindRegion(const TopologySnapshot &Snapshot, const std::wstring &Id)
{
    for (const auto &region : Snapshot.Regions)
    {
        if (region.Id == Id)
        {
            return &region;
        }
    }

    return nullptr;
}

} // namespace Transition