#pragma once

namespace Transition {

enum class DuplicationState
{
    Uninitialized,

    Active,

    Reinitializing,

    AccessLost,

    DeviceRemoved
};

} // namespace Transition