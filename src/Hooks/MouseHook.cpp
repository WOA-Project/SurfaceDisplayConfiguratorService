#include "pch.h"
#include "Hooks/MouseHook.h"
#include "Core/SpanController.h"

MouseHook *MouseHook::s_instance = nullptr;

MouseHook::MouseHook(SpanController &controller) : m_controller(controller)
{
    s_instance = this;
}

MouseHook::~MouseHook()
{
    Uninstall();
}

bool
MouseHook::Install()
{
    if (m_hook)
    {
        return true;
    }

    m_hook = SetWindowsHookExW(WH_MOUSE_LL, HookProc, GetModuleHandleW(nullptr), 0);

    return m_hook != nullptr;
}

void
MouseHook::Uninstall()
{
    if (m_hook)
    {
        UnhookWindowsHookEx(m_hook);

        m_hook = nullptr;
    }

    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

LRESULT CALLBACK
MouseHook::HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION)
    {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    auto *hook = s_instance;

    if (!hook)
    {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    if (!lParam)
    {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    switch (wParam)
    {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_NCLBUTTONDOWN: {
        const auto *mouse = reinterpret_cast<const MSLLHOOKSTRUCT *>(lParam);

        HWND hwnd = WindowFromPoint(mouse->pt);

        hook->m_controller.OnMouseClick(mouse->pt, hwnd);

        break;
    }

    default:
        break;
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}