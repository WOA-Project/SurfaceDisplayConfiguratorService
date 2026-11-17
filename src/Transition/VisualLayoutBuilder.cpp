#include "pch.h"

#include "Transition\\VisualLayoutBuilder.h"

#include <algorithm>

namespace Transition {

HRESULT
VisualLayoutBuilder::BuildTransition(
    const std::vector<VisualState> &CurrentStates,
    const std::vector<VisualState> &TargetStates,
    std::vector<VisualTransition> &Result)
{
    Result.clear();

    const size_t count = std::min(CurrentStates.size(), TargetStates.size());

    Result.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        VisualTransition transition{};

        transition.Current = CurrentStates[i];

        transition.Target = TargetStates[i];

        Result.emplace_back(std::move(transition));
    }

    return S_OK;
}

} // namespace Transition