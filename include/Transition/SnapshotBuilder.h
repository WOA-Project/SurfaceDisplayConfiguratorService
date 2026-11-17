#pragma once

#include "Transition/TopologySnapshot.h"

#include <InputStitching/Plan.h>

namespace Transition {

class SnapshotBuilder
{
public:
    static TopologySnapshot BuildTopologySnapshot(
        const wchar_t *LeftPanelId,
        const wchar_t *RightPanelId,
        const DEVMODE &LeftDevMode,
        const DEVMODE &RightDevMode,
        bool LeftEnabled,
        bool RightEnabled);
};

} // namespace Transition