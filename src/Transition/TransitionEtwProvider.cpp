#include "pch.h"

#include "Transition\TransitionEtwProvider.h"

#include <evntprov.h>
#include <evntrace.h>

#ifndef TRACE_LEVEL_INFORMATION
#    define TRACE_LEVEL_INFORMATION 4
#endif

#pragma comment(lib, "advapi32.lib")

namespace Transition {

REGHANDLE
TransitionEtwProvider::s_ProviderHandle = 0;

namespace {

// Stable provider GUID for the transition framework.
static const GUID g_ProviderGuid = {0x9a8f3f12, 0x8f7c, 0x4d66, {0xa1, 0x4c, 0x91, 0x77, 0xe2, 0x44, 0x53, 0xaa}};

void
WriteStringEvent(UCHAR EventId, LPCWSTR Message)
{
    if (!TransitionEtwProvider::s_ProviderHandle)
    {
        return;
    }

    EVENT_DATA_DESCRIPTOR descriptor;

    EventDataDescCreate(&descriptor, Message, static_cast<ULONG>((wcslen(Message) + 1) * sizeof(wchar_t)));

    EVENT_DESCRIPTOR eventDescriptor = {EventId, 0, 0, TRACE_LEVEL_INFORMATION, 0, 0, 0};

    EventWrite(TransitionEtwProvider::s_ProviderHandle, &eventDescriptor, 1, &descriptor);
}

} // namespace

HRESULT
TransitionEtwProvider::Register()
{
    const ULONG status = EventRegister(&g_ProviderGuid, nullptr, nullptr, &s_ProviderHandle);

    return HRESULT_FROM_WIN32(status);
}

void
TransitionEtwProvider::Unregister()
{
    if (s_ProviderHandle)
    {
        EventUnregister(s_ProviderHandle);

        s_ProviderHandle = 0;
    }
}

void
TransitionEtwProvider::TransitionBegin(LPCWSTR ScenarioName)
{
    WriteStringEvent(1, ScenarioName ? ScenarioName : L"TransitionBegin");
}

void
TransitionEtwProvider::TransitionEnd(LPCWSTR ScenarioName, UINT DurationMs)
{
    wchar_t buffer[256]{};

    swprintf_s(buffer, L"%s Duration=%u", ScenarioName ? ScenarioName : L"TransitionEnd", DurationMs);

    WriteStringEvent(2, buffer);
}

void
TransitionEtwProvider::CaptureBegin()
{
    WriteStringEvent(3, L"CaptureBegin");
}

void
TransitionEtwProvider::CaptureEnd(UINT DurationMs)
{
    wchar_t buffer[64]{};

    swprintf_s(buffer, L"CaptureEnd Duration=%u", DurationMs);

    WriteStringEvent(4, buffer);
}

void
TransitionEtwProvider::AnimationBegin(LPCWSTR ScenarioName)
{
    WriteStringEvent(5, ScenarioName ? ScenarioName : L"AnimationBegin");
}

void
TransitionEtwProvider::AnimationCommitPoint(LPCWSTR ScenarioName)
{
    WriteStringEvent(6, ScenarioName ? ScenarioName : L"AnimationCommitPoint");
}

void
TransitionEtwProvider::AnimationEnd(LPCWSTR ScenarioName)
{
    WriteStringEvent(7, ScenarioName ? ScenarioName : L"AnimationEnd");
}

void
TransitionEtwProvider::TopologyActionBegin()
{
    WriteStringEvent(8, L"TopologyActionBegin");
}

void
TransitionEtwProvider::TopologyActionEnd(HRESULT Hr)
{
    wchar_t buffer[64]{};

    swprintf_s(buffer, L"TopologyActionEnd HR=0x%08X", static_cast<unsigned>(Hr));

    WriteStringEvent(9, buffer);
}

void
TransitionEtwProvider::RebuildBegin()
{
    WriteStringEvent(10, L"RebuildBegin");
}

void
TransitionEtwProvider::RebuildEnd(UINT DurationMs)
{
    wchar_t buffer[64]{};

    swprintf_s(buffer, L"RebuildEnd Duration=%u", DurationMs);

    WriteStringEvent(11, buffer);
}

void
TransitionEtwProvider::AccessLost()
{
    WriteStringEvent(12, L"AccessLost");
}

void
TransitionEtwProvider::DeviceRemoved(HRESULT Reason)
{
    wchar_t buffer[64]{};

    swprintf_s(buffer, L"DeviceRemoved HR=0x%08X", static_cast<unsigned>(Reason));

    WriteStringEvent(13, buffer);
}

void
TransitionEtwProvider::TopologyChanged(UINT OutputCount)
{
    wchar_t buffer[64]{};

    swprintf_s(buffer, L"TopologyChanged Outputs=%u", OutputCount);

    WriteStringEvent(14, buffer);
}

} // namespace Transition
