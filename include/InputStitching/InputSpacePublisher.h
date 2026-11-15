#pragma once

#include "InputStitching\InputSpaceAbi.h"

namespace InputStitching {

struct Plan;

//
// Responsible for publishing validated
// input-space plans through
// NtConfigureInputSpace().
//
// This class is the only component that
// should interact with the undocumented
// ABI directly.
//

class InputSpacePublisher
{
public:
    //
    // Publish a complete input-space plan.
    //
    HRESULT Publish(const Plan &PlanData);

    //
    // Remove a previously published
    // input space.
    //
    HRESULT Remove(const LUID &SpaceId);

private:
    //
    // Resolve win32u!NtConfigureInputSpace.
    //
    PFN_NtConfigureInputSpace Resolve();

private:
    //
    // Cached procedure pointer.
    //
    PFN_NtConfigureInputSpace m_NtConfigureInputSpace = nullptr;
};

} // namespace InputStitching