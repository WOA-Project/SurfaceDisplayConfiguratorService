#pragma once

#include <windows.h>

#include <cstdint>

namespace Transition {

class AnimationClock
{
public:
    HRESULT Start(uint32_t DurationMs, float CommitPoint);

    HRESULT WaitForCommitPoint();

    HRESULT WaitForCompletion();

private:
    uint64_t m_StartTick = 0;

    uint32_t m_DurationMs = 0;

    float m_CommitPoint = 0.0f;

    bool m_Running = false;
};

} // namespace Transition