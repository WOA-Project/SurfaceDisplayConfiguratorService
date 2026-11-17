#pragma once

#include <windows.h>

#include <atomic>

namespace Transition {

enum class AnimationClockState
{
    Idle,

    Running,

    Completed,

    Cancelled
};

class AnimationClock
{
public:
    AnimationClock() = default;

    HRESULT Start(UINT DurationMs, float CommitPoint);

    void Cancel();

    bool IsRunning() const noexcept;

    bool IsCancelled() const noexcept;

    bool IsComplete() const noexcept;

    bool HasReachedCommitPoint() const noexcept;

    HRESULT WaitForCommitPoint(DWORD PollIntervalMs = 1);

    HRESULT WaitForCompletion(DWORD PollIntervalMs = 1);

    UINT GetDurationMs() const noexcept;

    UINT GetCommitTimeMs() const noexcept;

private:
    UINT m_DurationMs = 0;

    UINT m_CommitTimeMs = 0;

    ULONGLONG m_StartTick = 0;

    std::atomic<AnimationClockState> m_State = AnimationClockState::Idle;
};

} // namespace Transition