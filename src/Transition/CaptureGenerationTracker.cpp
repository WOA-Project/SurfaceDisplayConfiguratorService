#include "pch.h"

#include "Transition\CaptureGenerationTracker.h"

#include "Transition\TopologyGeneration.h"

namespace Transition {

CaptureGenerationTracker::CaptureGenerationTracker()
{
    Synchronize();
}

bool
CaptureGenerationTracker::IsStale() const
{
    return m_Generation != TopologyGeneration::Current();
}

void
CaptureGenerationTracker::Synchronize()
{
    m_Generation = TopologyGeneration::Current();
}

} // namespace Transition
