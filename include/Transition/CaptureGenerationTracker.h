#pragma once

#include <cstdint>

namespace Transition {

class CaptureGenerationTracker
{
public:
    CaptureGenerationTracker();

    bool IsStale() const;

    void Synchronize();

private:
    uint64_t m_Generation = 0;
};

} // namespace Transition