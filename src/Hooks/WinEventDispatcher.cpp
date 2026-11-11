#include "pch.h"
#include "Hooks/WinEventDispatcher.h"
#include "Core/SpanController.h"
#include "VirtualDesktop.h"

WinEventDispatcher *WinEventDispatcher::s_instance = nullptr;

WinEventDispatcher::WinEventDispatcher(SpanController &controller) : m_controller(controller)
{
    s_instance = this;
}

WinEventDispatcher::~WinEventDispatcher()
{
    Uninstall();
}

bool
WinEventDispatcher::Install()
{
    if (!m_hooks.empty())
    {
        return true;
    }

    constexpr DWORD events[] = {
        EVENT_SYSTEM_FOREGROUND,

        EVENT_OBJECT_NAMECHANGE,
        EVENT_OBJECT_UNCLOAKED,
        EVENT_OBJECT_SHOW,
        EVENT_OBJECT_CREATE,
        EVENT_OBJECT_DESTROY,

        EVENT_SYSTEM_MOVESIZESTART,
        EVENT_SYSTEM_MOVESIZEEND};

    for (DWORD eventId : events)
    {
        HWINEVENTHOOK hook = SetWinEventHook(
            eventId, eventId, nullptr, EventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

        if (hook)
        {
            m_hooks.push_back(hook);
        }
    }

    return !m_hooks.empty();
}

void
WinEventDispatcher::Uninstall()
{
    for (auto hook : m_hooks)
    {
        if (hook)
        {
            UnhookWinEvent(hook);
        }
    }

    m_hooks.clear();

    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

void CALLBACK
WinEventDispatcher::EventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG object, LONG child, DWORD, DWORD)
{
    UNREFERENCED_PARAMETER(child);

    auto *dispatcher = s_instance;

    if (!dispatcher)
    {
        return;
    }

    switch (event)
    {
    case EVENT_OBJECT_NAMECHANGE: {
        //
        // Virtual desktop switches trigger a
        // desktop name change notification.
        //
        if (hwnd == GetDesktopWindow())
        {
            VirtualDesktop::instance().UpdateVirtualDesktopId();
        }

        break;
    }

    case EVENT_OBJECT_CREATE:
    case EVENT_OBJECT_SHOW:
    case EVENT_OBJECT_UNCLOAKED: {
        if (object == OBJID_WINDOW && hwnd)
        {
            dispatcher->m_controller.OnWindowCreated(hwnd);
        }

        break;
    }

    case EVENT_SYSTEM_FOREGROUND: {
        if (hwnd)
        {
            dispatcher->m_controller.OnForegroundChanged(hwnd);
        }

        break;
    }

    case EVENT_SYSTEM_MOVESIZESTART: {
        if (hwnd)
        {
            dispatcher->m_controller.OnMoveSizeStart(hwnd);
        }

        break;
    }

    case EVENT_SYSTEM_MOVESIZEEND: {
        if (hwnd)
        {
            dispatcher->m_controller.OnMoveSizeEnd(hwnd);
        }

        break;
    }

    case EVENT_OBJECT_DESTROY: {
        if (object == OBJID_WINDOW && hwnd)
        {
            dispatcher->m_controller.OnWindowDestroyed(hwnd);
        }

        break;
    }

    default:
        break;
    }
}