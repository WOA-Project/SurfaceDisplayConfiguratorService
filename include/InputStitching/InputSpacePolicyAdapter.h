#pragma once

#include "InputStitching\InputSpacePolicy.h"
#include "InputStitching\InputSpaceConfiguration.h"
#include "OrientationManager.h"

namespace InputStitching {

InputSpacePolicy
BuildInputSpacePolicy(
    CONST WCHAR *DisplayPanelId1,
    CONST WCHAR *DisplayPanelId2,
    const DisplayPolicy &Display,
    const InputSpaceConfiguration &Configuration);

InputSpacePolicy
BuildInputSpacePolicyInternal(const DisplayPolicy &Display, const InputSpaceConfiguration &Configuration);

} // namespace InputStitching