#pragma once

#include <atomic>
#include <cstdint>

namespace Transition {

class TopologyGeneration
{
public:
    static uint64_t Current();

    static uint64_t Increment();

private:
    static std::atomic<uint64_t> s_Generation;
};

} // namespace Transition