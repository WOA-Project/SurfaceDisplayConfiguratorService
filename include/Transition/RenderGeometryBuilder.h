#pragma once

#include "Transition/RenderGeometry.h"
#include "Transition/DesktopCaptureManager.h"
#include "Transition/TopologySnapshot.h"

#include <vector>

namespace Transition {

class RenderGeometryBuilder
{
public:
    HRESULT Build(
        const TopologySnapshot &Snapshot,
        const CapturedDesktop &CapturedDesktop,
        std::vector<RenderGeometry> &Result);
};

} // namespace Transition