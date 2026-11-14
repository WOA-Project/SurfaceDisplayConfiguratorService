#pragma once

#include <windows.h>

class DisplayChangeMonitor
{
public:
    bool Start();
    void Stop();
    void QueueValidation();

private:
    static DWORD WINAPI ValidationWorker(void* Context);

    std::atomic<ULONG> m_validationGeneration{0};
    std::atomic<BOOL> m_validationRunning{FALSE};

    static DWORD WINAPI ThreadProc(void* Context);
    static LRESULT CALLBACK WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam);

    HANDLE m_thread = nullptr;
    DWORD m_threadId = 0;
    HWND m_hwnd = nullptr;
    HANDLE m_stopEvent = nullptr;
};
