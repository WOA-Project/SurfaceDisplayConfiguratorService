#pragma once

#include "InputStitching\InputSpacePolicy.h"

namespace InputStitching {

struct Plan;

//
// High-level runtime entry point for the
// Input Space subsystem.
//
// Responsibilities:
//
//   Store current policy
//   Build plans
//   Validate plans
//   Publish plans
//   Remove plans
//   Rebuild plans after topology changes
//

class InputSpaceManager
{
public:
    //
    // Initialize the subsystem.
    //
    HRESULT Initialize();

    //
    // Apply a new policy and rebuild the
    // current plan.
    //
    HRESULT ApplyPolicy(const InputSpacePolicy &Policy);

    //
    // Publish the current validated plan
    // through NtConfigureInputSpace().
    //
    HRESULT Publish();

    //
    // Remove the currently published
    // input space.
    //
    HRESULT Remove();

    //
    // Rebuild the current plan using the
    // most recently applied policy.
    //
    HRESULT Rebuild();

    //
    // Access the current plan.
    //
    const Plan &CurrentPlan() const;

private:
    //
    // Last successfully applied policy.
    //
    InputSpacePolicy m_Policy{};
};

} // namespace InputStitching