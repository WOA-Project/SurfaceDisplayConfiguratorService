#include "pch.h"
#include "Application/Runtime.h"

#include <objbase.h>
#include <shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")

namespace {
constexpr UINT_PTR PreviewTimerId = 1;
constexpr UINT PreviewTimerMs = 50;
constexpr wchar_t RuntimeWindowClassName[] = L"SurfaceDisplayConfiguratorRuntime";
constexpr auto RutimeObjectProp = L"SurfaceDisplayConfiguratorRuntimeProp";
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

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = RuntimeWindowClassName;

    RegisterClassExW(&wc);

    m_hiddenWindow = winrt::check_pointer(CreateWindowExW(
        WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
        RuntimeWindowClassName,
        L"",
        WS_OVERLAPPED,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr));

    SetPropW(m_hiddenWindow, RutimeObjectProp, this);

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

    m_previewTimerId = SetTimer(m_hiddenWindow, 1, PreviewTimerMs, nullptr);

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
        KillTimer(m_hiddenWindow, m_previewTimerId);

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
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

LRESULT CALLBACK
Runtime::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto _this = reinterpret_cast<Runtime *>(GetPropW(hwnd, RutimeObjectProp));

    if (msg == WM_DESTROY)
    {
        RemovePropW(hwnd, RutimeObjectProp);
    }
    else if (_this)
    {
        switch (msg)
        {
        case WM_DISPLAYCHANGE: {
            _this->m_controller.OnDisplayConfigurationChanged();

            break;
        }

        case WM_SETTINGCHANGE: {
            if (wParam == SPI_SETWORKAREA)
            {
                _this->m_controller.OnDisplayConfigurationChanged();
            }

            break;
        }

        case WM_TIMER: {
            if (wParam == _this->m_previewTimerId)
            {
                _this->m_controller.UpdateDragPreview();
            }

            break;
        }
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
