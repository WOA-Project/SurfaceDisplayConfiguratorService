#pragma once

#include <Windows.h>

class SpanController;

class MouseHook
{
public:
    explicit MouseHook(SpanController &controller);

    ~MouseHook();

    MouseHook(const MouseHook &) = delete;

    MouseHook &operator=(const MouseHook &) = delete;

    MouseHook(MouseHook &&) = delete;

    MouseHook &operator=(MouseHook &&) = delete;

    bool Install();

    void Uninstall();

private:
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    static MouseHook *s_instance;

    SpanController &m_controller;

    HHOOK
    m_hook{};
};
