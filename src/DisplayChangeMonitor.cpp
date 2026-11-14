#include "pch.h"

#include "DisplayChangeMonitor.h"
#include "DisplayRotationManager.h"

DWORD WINAPI
DisplayChangeMonitor::ThreadProc(void *Context)
{
    auto self = static_cast<DisplayChangeMonitor *>(Context);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"DuoDisplayChangeMonitor";

    RegisterClassExW(&wc);

    self->m_hwnd =
        CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, wc.hInstance, self);

    MSG msg;

    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

LRESULT CALLBACK
DisplayChangeMonitor::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DisplayChangeMonitor *self = reinterpret_cast<DisplayChangeMonitor *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_NCCREATE: {
        auto cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return TRUE;
    }

    case WM_DISPLAYCHANGE: {
        if (self)
        {
            self->QueueValidation();
        }
        return 0;
    }

    case WM_DEVICECHANGE: {
        wprintf(L"WM_DEVICECHANGE\n");
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool
DisplayChangeMonitor::Start()
{
    m_thread = CreateThread(nullptr, 0, ThreadProc, this, 0, &m_threadId);
    return m_thread != nullptr;
}

void
DisplayChangeMonitor::Stop()
{
    if (m_hwnd)
    {
        PostMessageW(m_hwnd, WM_QUIT, 0, 0);
    }

    if (m_thread)
    {
        WaitForSingleObject(m_thread, INFINITE);

        CloseHandle(m_thread);
        m_thread = nullptr;
    }
}

void
DisplayChangeMonitor::QueueValidation()
{
    m_validationGeneration.fetch_add(1, std::memory_order_relaxed);

    BOOL Expected = FALSE;

    if (m_validationRunning.compare_exchange_strong(Expected, TRUE))
    {
        QueueUserWorkItem(ValidationWorker, this, WT_EXECUTEDEFAULT);
    }
}

DWORD WINAPI
DisplayChangeMonitor::ValidationWorker(void *Context)
{
    auto self = static_cast<DisplayChangeMonitor *>(Context);

    ULONG ObservedGeneration = self->m_validationGeneration.load(std::memory_order_acquire);

    for (;;)
    {
        //
        // Wait for display topology/orientation
        // changes to settle.
        //
        Sleep(1500);

        ULONG CurrentGeneration = self->m_validationGeneration.load(std::memory_order_acquire);

        //
        // Another WM_DISPLAYCHANGE arrived while
        // we were waiting. Restart debounce.
        //
        if (CurrentGeneration != ObservedGeneration)
        {
            ObservedGeneration = CurrentGeneration;

            continue;
        }

        break;
    }

    //
    // Perform validation once the display stack
    // appears stable.
    //
    ValidateDisplayConfiguration();

    self->m_validationRunning.store(FALSE, std::memory_order_release);

    //
    // A display change could have happened while
    // validation was running.
    //
    if (self->m_validationGeneration.load(std::memory_order_acquire) != ObservedGeneration)
    {
        self->QueueValidation();
    }

    return 0;
}