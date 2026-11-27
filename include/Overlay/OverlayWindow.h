#pragma once
#include <windows.system.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>

using namespace winrt;
using namespace Windows::System;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;

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

    void Destroy();

    bool IsVisible() const;

    HWND GetHandle() const;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_window{};

    bool m_visible{};

    DispatcherQueueController m_dispatcherQueueController{nullptr};

    Compositor m_compositor{nullptr};

    DesktopWindowTarget m_target{nullptr};

    ContainerVisual m_rootVisual{nullptr};

    ContainerVisual m_hostVisual{nullptr};
};