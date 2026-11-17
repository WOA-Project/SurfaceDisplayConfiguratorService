#pragma once

#include <windows.h>

#include <string>
#include <string_view>

namespace Transition {

enum class TraceLevel
{
    Verbose,
    Information,
    Warning,
    Error
};

enum class TraceEvent
{
    TransitionBegin,
    TransitionEnd,

    CaptureBegin,
    CaptureEnd,

    AnimationBegin,
    AnimationCommitPoint,
    AnimationEnd,

    TopologyActionBegin,
    TopologyActionEnd,

    RebuildBegin,
    RebuildEnd,

    AccessLost,

    DeviceRemoved,

    TopologyChanged
};

class TransitionTrace
{
public:
    static void Write(TraceLevel Level, TraceEvent Event, std::wstring_view Message);

    static void WriteHr(TraceLevel Level, TraceEvent Event, HRESULT Hr, std::wstring_view Message);

    static ULONGLONG Timestamp() noexcept;
};

class ScopedTrace
{
public:
    ScopedTrace(TraceEvent BeginEvent, TraceEvent EndEvent, std::wstring_view Name);

    ~ScopedTrace();

    ScopedTrace(const ScopedTrace &) = delete;

    ScopedTrace &operator=(const ScopedTrace &) = delete;

private:
    TraceEvent m_EndEvent;

    std::wstring m_Name;

    ULONGLONG m_StartTick = 0;
};

} // namespace Transition