#pragma once

#include <Windows.h>

//
// Mixed-orientation postures (tent mode, inward V mode, etc.)
// require Windows auto-rotation to be disabled because each
// display may need a different orientation.
//

VOID UpdateAutoRotationPolicy(
    INT LeftOrientation,
    INT RightOrientation,
    BOOLEAN LeftEnabled,
    BOOLEAN RightEnabled);