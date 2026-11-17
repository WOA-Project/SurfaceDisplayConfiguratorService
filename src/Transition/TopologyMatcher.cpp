#include "pch.h"

#include "Transition\\TopologyMatcher.h"

#include <algorithm>

namespace Transition {

HRESULT
TopologyMatcher::Match(const TopologySnapshot &Current, const TopologySnapshot &Target, std::vector<RegionPair> &Result)
{
    Result.clear();

    std::vector<bool> targetMatched(Target.Regions.size(), false);

    for (const auto &currentRegion : Current.Regions)
    {
        RegionPair pair{};

        bool foundMatch = false;

        for (size_t i = 0; i < Target.Regions.size(); ++i)
        {
            if (targetMatched[i])
            {
                continue;
            }

            if (Target.Regions[i].Id == currentRegion.Id)
            {
                pair.Current.Id = currentRegion.Id;

                pair.Current.Role = currentRegion.Role;

                pair.Current.Rect = currentRegion.DesktopRect;

                pair.Current.DesktopRect = currentRegion.DesktopRect;

                pair.Current.RotationDegrees = 0.0f;

                pair.Current.Opacity = 1.0f;

                pair.Target.Id = Target.Regions[i].Id;

                pair.Target.Role = Target.Regions[i].Role;

                pair.Target.Rect = Target.Regions[i].DesktopRect;

                pair.Target.DesktopRect = Target.Regions[i].DesktopRect;

                pair.Target.RotationDegrees = 0.0f;

                pair.Target.Opacity = 1.0f;

                targetMatched[i] = true;

                foundMatch = true;

                break;
            }
        }

        if (foundMatch)
        {
            Result.emplace_back(std::move(pair));
        }
    }

    return S_OK;
}

} // namespace Transition