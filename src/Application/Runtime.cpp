#include "pch.h"
#include "Application/Runtime.h"

#include <objbase.h>
#include <shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")

namespace {
constexpr UINT_PTR PreviewTimerId = 1;
constexpr UINT PreviewTimerMs = 50;
} // namespace

Runtime::Runtime() {}

Runtime::~Runtime()
{
    Shutdown();
}

void
Runtime::Run()
{
    if (!Initialize())
    {
        return;
    }

    MessageLoop();
}

bool
Runtime::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    if (SUCCEEDED(CoInitialize(nullptr)))
    {
        m_comInitialized = true;
    }

    //
    // May fail if another component
    // already established process DPI
    // awareness. That's acceptable.
    //
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    if (!m_controller.Initialize())
    {
        Shutdown();
        return false;
    }

    m_eventDispatcher = std::make_unique<WinEventDispatcher>(m_controller);

    m_mouseHook = std::make_unique<MouseHook>(m_controller);

    if (!m_eventDispatcher || !m_eventDispatcher->Install())
    {
        Shutdown();
        return false;
    }

    if (!m_mouseHook || !m_mouseHook->Install())
    {
        Shutdown();
        return false;
    }

    m_previewTimerId = SetTimer(nullptr, 0, PreviewTimerMs, nullptr);

    if (!m_previewTimerId)
    {
        Shutdown();
        return false;
    }

    m_initialized = true;

    return true;
}

void
Runtime::Shutdown()
{
    if (!m_initialized && !m_comInitialized)
    {
        return;
    }

    if (m_previewTimerId)
    {
        KillTimer(nullptr, m_previewTimerId);

        m_previewTimerId = 0;
    }

    if (m_mouseHook)
    {
        m_mouseHook->Uninstall();
        m_mouseHook.reset();
    }

    if (m_eventDispatcher)
    {
        m_eventDispatcher->Uninstall();
        m_eventDispatcher.reset();
    }

    m_controller.Shutdown();

    if (m_comInitialized)
    {
        CoUninitialize();

        m_comInitialized = false;
    }

    m_initialized = false;
}

void
Runtime::MessageLoop()
{
    MSG message{};

    while (GetMessage(&message, nullptr, 0, 0))
    {
        switch (message.message)
        {
        case WM_DISPLAYCHANGE: {
            m_controller.OnDisplayConfigurationChanged();

            break;
        }

        case WM_SETTINGCHANGE: {
            if (message.wParam == SPI_SETWORKAREA)
            {
                m_controller.OnDisplayConfigurationChanged();
            }

            break;
        }

        case WM_TIMER: {
            if (message.wParam == m_previewTimerId)
            {
                m_controller.UpdateDragPreview();
            }

            break;
        }

        default:
            break;
        }

        TranslateMessage(&message);

        DispatchMessage(&message);
    }
}