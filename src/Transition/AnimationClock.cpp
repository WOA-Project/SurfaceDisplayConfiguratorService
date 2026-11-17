#include "pch.h"

#include "Transition\AnimationClock.h"

namespace Transition {

HRESULT
AnimationClock::Start(UINT DurationMs, float CommitPoint)
{
    if (DurationMs == 0)
    {
        return E_INVALIDARG;
    }

    if (CommitPoint < 0.0f || CommitPoint > 1.0f)
    {
        return E_INVALIDARG;
    }

    m_DurationMs = DurationMs;

    m_CommitTimeMs = static_cast<UINT>(static_cast<float>(DurationMs) * CommitPoint);

    m_StartTick = GetTickCount64();

    m_State.store(AnimationClockState::Running, std::memory_order_release);

    return S_OK;
}

void
AnimationClock::Cancel()
{
    m_State.store(AnimationClockState::Cancelled, std::memory_order_release);
}

bool
AnimationClock::IsRunning() const noexcept
{
    return m_State.load(std::memory_order_acquire) == AnimationClockState::Running;
}

bool
AnimationClock::IsCancelled() const noexcept
{
    return m_State.load(std::memory_order_acquire) == AnimationClockState::Cancelled;
}

bool
AnimationClock::IsComplete() const noexcept
{
    const auto state = m_State.load(std::memory_order_acquire);

    if (state == AnimationClockState::Cancelled)
    {
        return true;
    }

    if (state == AnimationClockState::Completed)
    {
        return true;
    }

    if (state != AnimationClockState::Running)
    {
        return false;
    }

    const ULONGLONG elapsed = GetTickCount64() - m_StartTick;

    if (elapsed >= m_DurationMs)
    {
        const_cast<AnimationClock *>(this)->m_State.store(AnimationClockState::Completed, std::memory_order_release);

        return true;
    }

    return false;
}

bool
AnimationClock::HasReachedCommitPoint() const noexcept
{
    if (!IsRunning())
    {
        return false;
    }

    const ULONGLONG elapsed = GetTickCount64() - m_StartTick;

    return elapsed >= m_CommitTimeMs;
}

HRESULT
AnimationClock::WaitForCommitPoint(DWORD PollIntervalMs)
{
    while (!HasReachedCommitPoint())
    {
        if (IsCancelled())
        {
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }

        Sleep(PollIntervalMs);
    }

    return S_OK;
}

HRESULT
AnimationClock::WaitForCompletion(DWORD PollIntervalMs)
{
    while (!IsComplete())
    {
        if (IsCancelled())
        {
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }

        Sleep(PollIntervalMs);
    }

    return S_OK;
}

UINT
AnimationClock::GetDurationMs() const noexcept
{
    return m_DurationMs;
}

UINT
AnimationClock::GetCommitTimeMs() const noexcept
{
    return m_CommitTimeMs;
}

} // namespace Transition
