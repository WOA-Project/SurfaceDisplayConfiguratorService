#include "pch.h"

#include "Transition\TransitionTrace.h"

#include "Transition\TransitionStatistics.h"

#include <cstdio>

namespace Transition {

namespace {

const wchar_t *
ToString(TraceLevel Level)
{
    switch (Level)
    {
    case TraceLevel::Verbose:
        return L"VERBOSE";

    case TraceLevel::Information:
        return L"INFO";

    case TraceLevel::Warning:
        return L"WARN";

    case TraceLevel::Error:
        return L"ERROR";

    default:
        return L"?";
    }
}

const wchar_t *
ToString(TraceEvent Event)
{
    switch (Event)
    {
    case TraceEvent::TransitionBegin:
        return L"TransitionBegin";

    case TraceEvent::TransitionEnd:
        return L"TransitionEnd";

    case TraceEvent::CaptureBegin:
        return L"CaptureBegin";

    case TraceEvent::CaptureEnd:
        return L"CaptureEnd";

    case TraceEvent::AnimationBegin:
        return L"AnimationBegin";

    case TraceEvent::AnimationCommitPoint:
        return L"AnimationCommitPoint";

    case TraceEvent::AnimationEnd:
        return L"AnimationEnd";

    case TraceEvent::TopologyActionBegin:
        return L"TopologyActionBegin";

    case TraceEvent::TopologyActionEnd:
        return L"TopologyActionEnd";

    case TraceEvent::RebuildBegin:
        return L"RebuildBegin";

    case TraceEvent::RebuildEnd:
        return L"RebuildEnd";

    case TraceEvent::AccessLost:
        return L"AccessLost";

    case TraceEvent::DeviceRemoved:
        return L"DeviceRemoved";

    case TraceEvent::TopologyChanged:
        return L"TopologyChanged";

    default:
        return L"?";
    }
}

} // namespace

ULONGLONG
TransitionTrace::Timestamp() noexcept
{
    return GetTickCount64();
}

void
TransitionTrace::Write(TraceLevel Level, TraceEvent Event, std::wstring_view Message)
{
    wchar_t buffer[1024]{};

    swprintf_s(
        buffer,
        L"[Transition][%s][%s] %.*s\r\n",
        ToString(Level),
        ToString(Event),
        static_cast<int>(Message.size()),
        Message.data());

    wprintf(buffer);
}

void
TransitionTrace::WriteHr(TraceLevel Level, TraceEvent Event, HRESULT Hr, std::wstring_view Message)
{
    wchar_t buffer[1024]{};

    swprintf_s(
        buffer,
        L"[Transition][%s][%s] hr=0x%08X %.*s\r\n",
        ToString(Level),
        ToString(Event),
        static_cast<unsigned>(Hr),
        static_cast<int>(Message.size()),
        Message.data());

    wprintf(buffer);
}

ScopedTrace::ScopedTrace(TraceEvent BeginEvent, TraceEvent EndEvent, std::wstring_view Name) :
    m_EndEvent(EndEvent), m_Name(Name), m_StartTick(GetTickCount64())
{
    TransitionTrace::Write(TraceLevel::Information, BeginEvent, Name);
}

ScopedTrace::~ScopedTrace()
{
    const ULONGLONG durationMs = GetTickCount64() - m_StartTick;

    wchar_t buffer[512]{};

    swprintf_s(buffer, L"%s duration=%llu ms", m_Name.c_str(), durationMs);

    TransitionTrace::Write(TraceLevel::Information, m_EndEvent, buffer);

    if (m_EndEvent == TraceEvent::TransitionEnd)
    {
        TransitionStatistics::RecordTransition(durationMs);
    }
    else if (m_EndEvent == TraceEvent::CaptureEnd)
    {
        TransitionStatistics::RecordCapture(durationMs);
    }
    else if (m_EndEvent == TraceEvent::RebuildEnd)
    {
        TransitionStatistics::RecordRebuild(durationMs);
    }
}

} // namespace Transition
