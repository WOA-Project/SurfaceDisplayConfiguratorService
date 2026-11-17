#include "pch.h"

#include "Transition\\DisplayRegionMatcher.h"

#include <algorithm>

namespace Transition {

HRESULT
DisplayRegionMatcher::Match(
    const std::vector<VisualState> &CurrentStates,
    const std::vector<VisualState> &TargetStates,
    std::vector<RegionPair> &Result)
{
    Result.clear();

    std::vector<bool> targetMatched(TargetStates.size(), false);

    for (const auto &current : CurrentStates)
    {
        RegionPair pair{};

        pair.Current = current;

        bool foundMatch = false;

        for (size_t i = 0; i < TargetStates.size(); ++i)
        {
            if (targetMatched[i])
            {
                continue;
            }

            if (TargetStates[i].Id == current.Id)
            {
                pair.Target = TargetStates[i];

                targetMatched[i] = true;

                foundMatch = true;

                break;
            }
        }

        if (!foundMatch)
        {
            pair.Target = current;
        }

        Result.emplace_back(std::move(pair));
    }

    for (size_t i = 0; i < TargetStates.size(); ++i)
    {
        if (targetMatched[i])
        {
            continue;
        }

        RegionPair pair{};

        pair.Target = TargetStates[i];

        Result.emplace_back(std::move(pair));
    }

    return S_OK;
}

} // namespace Transition