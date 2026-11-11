#pragma once

#include <Windows.h>

#include <vector>

class SpanController;

class WinEventDispatcher
{
public:
    explicit WinEventDispatcher(SpanController &controller);

    ~WinEventDispatcher();

    WinEventDispatcher(const WinEventDispatcher &) = delete;

    WinEventDispatcher &operator=(const WinEventDispatcher &) = delete;

    WinEventDispatcher(WinEventDispatcher &&) = delete;

    WinEventDispatcher &operator=(WinEventDispatcher &&) = delete;

    bool Install();

    void Uninstall();

private:
    static void CALLBACK
    EventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG object, LONG child, DWORD thread, DWORD time);

private:
    static WinEventDispatcher *s_instance;

    SpanController &m_controller;

    std::vector<HWINEVENTHOOK> m_hooks;
};
