#pragma once

#include <Windows.h>

class OverlayWindow
{
public:
    OverlayWindow() = default;

    ~OverlayWindow();

    OverlayWindow(const OverlayWindow &) = delete;

    OverlayWindow &operator=(const OverlayWindow &) = delete;

    OverlayWindow(OverlayWindow &&) = delete;

    OverlayWindow &operator=(OverlayWindow &&) = delete;

    bool Create(HINSTANCE instance);

    void Show(const RECT &rect, HWND insertAfter);

    void Hide();

    bool IsVisible() const;

    HWND GetHandle() const;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_window{};

    bool m_visible{};
};