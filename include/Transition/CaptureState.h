#pragma once

namespace Transition {

enum class CaptureState
{
    Active,

    TopologyChanging,

    Rebuilding,

    Suspended
};

} // namespace Transition