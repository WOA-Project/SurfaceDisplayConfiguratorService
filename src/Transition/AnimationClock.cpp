#include "pch.h"

#include "Transition\\AnimationClock.h"

namespace Transition {

HRESULT
AnimationClock::Start(uint32_t DurationMs, float CommitPoint)
{
    if (CommitPoint < 0.0f || CommitPoint > 1.0f)
    {
        return E_INVALIDARG;
    }

    m_DurationMs = DurationMs;

    m_CommitPoint = CommitPoint;

    m_StartTick = GetTickCount64();

    m_Running = true;

    return S_OK;
}

HRESULT
AnimationClock::WaitForCommitPoint()
{
    if (!m_Running)
    {
        return E_UNEXPECTED;
    }

    const uint64_t commitDelayMs =
        static_cast<uint64_t>(static_cast<double>(m_DurationMs) * static_cast<double>(m_CommitPoint));

    const uint64_t now = GetTickCount64();

    const uint64_t elapsed = now - m_StartTick;

    if (elapsed < commitDelayMs)
    {
        Sleep(static_cast<DWORD>(commitDelayMs - elapsed));
    }

    return S_OK;
}

HRESULT
AnimationClock::WaitForCompletion()
{
    if (!m_Running)
    {
        return E_UNEXPECTED;
    }

    const uint64_t now = GetTickCount64();

    const uint64_t elapsed = now - m_StartTick;

    if (elapsed < m_DurationMs)
    {
        Sleep(static_cast<DWORD>(m_DurationMs - elapsed));
    }

    m_Running = false;

    return S_OK;
}

} // namespace Transition