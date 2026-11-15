#pragma once

#include "InputStitching\InputSpacePolicy.h"
#include "InputStitching\InputSpaceTypes.h"
#include "InputStitching\InputSpaceDisplayEnumerator.h"

namespace InputStitching {

struct Plan;

//
// Converts display topology into an
// INPUT_SPACE publication plan.
//
// Build flow:
//
//     EnumerateDisplays()
//               ↓
//     Resolve Left/Right panels
//               ↓
//     Apply mode filtering
//               ↓
//     Determine topology
//               ↓
//     Build display regions
//               ↓
//     Build optional gap region
//               ↓
//     Populate metadata
//               ↓
//     Produce Plan
//

class InputSpaceBuilder
{
public:
    HRESULT BuildPlan(const InputSpacePolicy &Policy, Plan *PlanData);

private:
    HRESULT GenerateLegacyPlan(const InputSpacePolicy &Policy, Plan *PlanData);

    HRESULT GenerateDualIndependentPlan(const InputSpacePolicy &Policy, Plan *PlanData);

    HRESULT GenerateDualStitchedPlan(const InputSpacePolicy &Policy, Plan *PlanData);

    HRESULT GenerateSingleLeftPlan(const InputSpacePolicy &Policy, Plan *PlanData);

    HRESULT GenerateSingleRightPlan(const InputSpacePolicy &Policy, Plan *PlanData);

private:
    HRESULT BuildDisplayRegions(const InputSpacePolicy &Policy, Plan *PlanData);

    HRESULT BuildGapRegion(const InputSpacePolicy &Policy, Plan *PlanData);

private:
    bool ShouldEnableStitching(const InputSpacePolicy &Policy, const Plan &PlanData) const;

    InternalPanelTopology DetermineTopology(const Plan &PlanData) const;

    DeviceArrangement DetermineArrangement(const DisplayEntry &Left, const DisplayEntry &Right);
};

} // namespace InputStitching