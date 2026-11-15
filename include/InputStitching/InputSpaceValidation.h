#pragma once

#include "InputStitching\InputSpaceAbi.h"

#include <string>
#include <vector>

namespace InputStitching {

struct Plan;

//
// Validates a generated input-space plan
// before publication through
// NtConfigureInputSpace().
//

HRESULT
ValidatePlan(const Plan &PlanData, std::wstring *Reason);

//
// Performs ABI-level validation intended
// to mirror the behavior observed in
// win32k input-space validation paths.
//
// Checks:
//
//   Space validity
//   Region validity
//   Flag combinations
//   Geometry sanity
//

bool
ValidateLikeWin32k(const INPUT_SPACE &Space, const std::vector<INPUT_SPACE_REGION> &Regions, std::wstring *Reason);

//
// Region-level validation helpers.
//

bool
ValidateRegion(const INPUT_SPACE_REGION &Region, std::wstring *Reason);

bool
ValidateDisplayRegion(const INPUT_SPACE_REGION &Region, std::wstring *Reason);

bool
ValidateCustomRegion(const INPUT_SPACE_REGION &Region, std::wstring *Reason);

//
// Geometry validation.
//

bool
ValidateRect(const RECT &Rect);

bool
ValidateModeRect(const INPUT_SPACE_REGION &Region);

bool
ValidateScalingRect(const INPUT_SPACE_REGION &Region);

//
// Rotation validation.
//

bool
ValidateRotation(UINT32 Rotation);

//
// DPI validation.
//

bool
ValidateDpi(const INPUT_SPACE_REGION &Region);

} // namespace InputStitching