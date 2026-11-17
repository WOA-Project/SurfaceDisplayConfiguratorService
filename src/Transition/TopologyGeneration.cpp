#include "pch.h"

#include "Transition\TopologyGeneration.h"

namespace Transition {

std::atomic<uint64_t> TopologyGeneration::s_Generation{1};

uint64_t
TopologyGeneration::Current()
{
    return s_Generation.load(std::memory_order_acquire);
}

uint64_t
TopologyGeneration::Increment()
{
    return s_Generation.fetch_add(1, std::memory_order_acq_rel) + 1;
}

} // namespace Transition
