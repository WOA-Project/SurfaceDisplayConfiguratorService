#pragma once

#include <windows.h>

namespace Transition {

class TransitionOverlayWindow
{
public:
    TransitionOverlayWindow() = default;
    ~TransitionOverlayWindow();

    TransitionOverlayWindow(const TransitionOverlayWindow &) = delete;

    TransitionOverlayWindow &operator=(const TransitionOverlayWindow &) = delete;

    HRESULT Create();

    void Destroy();

    void Show();

    void Hide();

    HWND GetWindow() const noexcept { return m_Window; }

    const RECT &GetVirtualDesktopRect() const noexcept { return m_VirtualDesktopRect; }

private:
    static LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

    HRESULT RegisterClass();

    void RefreshVirtualDesktopRect();

private:
    HWND m_Window = nullptr;

    RECT m_VirtualDesktopRect{};

    bool m_ClassRegistered = false;
};

} // namespace Transition