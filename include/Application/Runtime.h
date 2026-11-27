#pragma once

#include "../Core/SpanController.h"

#include "../Hooks/WinEventDispatcher.h"
#include "../Hooks/MouseHook.h"

#include <memory>

class Runtime
{
public:
    Runtime();

    ~Runtime();

    void Run();

private:
    bool Initialize();

    void Shutdown();

    void MessageLoop();

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    UINT_PTR m_previewTimerId{};

    SpanController m_controller;

    std::unique_ptr<WinEventDispatcher> m_eventDispatcher;

    std::unique_ptr<MouseHook> m_mouseHook;

    bool m_initialized{};

    bool m_comInitialized{};

    HWND m_hiddenWindow{};
};