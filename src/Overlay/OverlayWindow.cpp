#include "pch.h"
#include "Overlay/OverlayWindow.h"

#include <dwmapi.h>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Msimg32.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE

enum DWM_WINDOW_CORNER_PREFERENCE
{
    DWMWCP_DEFAULT = 0,
    DWMWCP_DONOTROUND = 1,
    DWMWCP_ROUND = 2,
    DWMWCP_ROUNDSMALL = 3
};

#    define DWMWA_WINDOW_CORNER_PREFERENCE 33

#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE

#    define DWMWA_SYSTEMBACKDROP_TYPE 38

enum DWM_SYSTEMBACKDROP_TYPE
{
    DWMSBT_AUTO = 0,
    DWMSBT_NONE = 1,
    DWMSBT_MAINWINDOW = 2,
    DWMSBT_TRANSIENTWINDOW = 3,
    DWMSBT_TABBEDWINDOW = 4
};

#endif

enum ACCENT_STATE
{
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_ENABLE_HOSTBACKDROP = 5
};

struct ACCENT_POLICY
{
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
};

enum WINDOWCOMPOSITIONATTRIB
{
    WCA_ACCENT_POLICY = 19
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

using SetWindowCompositionAttribute_t = BOOL(WINAPI *)(HWND, WINDOWCOMPOSITIONATTRIBDATA *);

namespace {
constexpr wchar_t OverlayClassName[] = L"SurfaceDisplayConfiguratorSpanOverlay";

SetWindowCompositionAttribute_t
GetCompositionFunction()
{
    static auto fn = reinterpret_cast<SetWindowCompositionAttribute_t>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

    return fn;
}

void
ApplyBackdrop(HWND hwnd)
{
    if (auto composition = GetCompositionFunction())
    {
        ACCENT_POLICY policy{};

        policy.AccentState = ACCENT_ENABLE_BLURBEHIND;

        WINDOWCOMPOSITIONATTRIBDATA data{};

        data.Attrib = WCA_ACCENT_POLICY;

        data.pvData = &policy;

        data.cbData = sizeof(policy);

        composition(hwnd, &data);
    }

    DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_TRANSIENTWINDOW;

    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
}
} // namespace

OverlayWindow::~OverlayWindow()
{
    if (m_window)
    {
        DestroyWindow(m_window);

        m_window = nullptr;
    }
}

bool
OverlayWindow::Create(HINSTANCE instance)
{
    if (m_window)
    {
        return true;
    }

    WNDCLASSEXW wc{};

    wc.cbSize = sizeof(wc);

    wc.lpfnWndProc = WindowProc;

    wc.hInstance = instance;

    wc.lpszClassName = OverlayClassName;

    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        return false;
    }

    m_window = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,

        OverlayClassName,
        L"",

        WS_POPUP,

        0,
        0,
        0,
        0,

        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!m_window)
    {
        return false;
    }

    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;

    DwmSetWindowAttribute(m_window, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    MARGINS margins = {-1};

    DwmExtendFrameIntoClientArea(m_window, &margins);

    ApplyBackdrop(m_window);

    return true;
}

void
OverlayWindow::Show(const RECT &rect, HWND insertAfter)
{
    if (!m_window)
    {
        return;
    }

    constexpr int inset = 16;

    const LONG width = std::max<LONG>(1, (rect.right - rect.left) - inset * 2);

    const LONG height = std::max<LONG>(1, (rect.bottom - rect.top) - inset * 2);

    SetWindowRgn(m_window, nullptr, FALSE);

    HRGN region = CreateRoundRectRgn(0, 0, width, height, 48, 48);

    SetWindowRgn(m_window, region, TRUE);

    SetWindowPos(
        m_window,
        insertAfter,

        rect.left + inset,
        rect.top + inset,

        width,
        height,

        SWP_SHOWWINDOW | SWP_NOACTIVATE);

    InvalidateRect(m_window, nullptr, TRUE);

    m_visible = true;
}

void
OverlayWindow::Hide()
{
    if (!m_window)
    {
        return;
    }

    ShowWindow(m_window, SW_HIDE);

    m_visible = false;
}

bool
OverlayWindow::IsVisible() const
{
    return m_visible;
}

HWND
OverlayWindow::GetHandle() const
{
    return m_window;
}

LRESULT CALLBACK
OverlayWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc{};
        GetClientRect(hwnd, &rc);

        HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));

        BLENDFUNCTION blend{};
        blend.BlendOp = AC_SRC_OVER;

        blend.SourceConstantAlpha = 12;

        HDC memDc = CreateCompatibleDC(hdc);

        HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);

        auto oldBmp = SelectObject(memDc, bmp);

        FillRect(memDc, &rc, brush);

        AlphaBlend(hdc, 0, 0, rc.right, rc.bottom, memDc, 0, 0, rc.right, rc.bottom, blend);

        SelectObject(memDc, oldBmp);

        DeleteObject(bmp);
        DeleteObject(brush);
        DeleteDC(memDc);

        EndPaint(hwnd, &ps);

        return 0;
    }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}