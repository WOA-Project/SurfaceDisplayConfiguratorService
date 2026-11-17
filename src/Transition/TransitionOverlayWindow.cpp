#include "pch.h"

#include "Transition\TransitionOverlayWindow.h"

namespace Transition {

namespace {

constexpr wchar_t kWindowClassName[] = L"TransitionOverlayWindow";

}

TransitionOverlayWindow::~TransitionOverlayWindow()
{
    Destroy();
}

HRESULT
TransitionOverlayWindow::RegisterClass()
{
    if (m_ClassRegistered)
    {
        return S_OK;
    }

    WNDCLASSEXW wc{};

    wc.cbSize = sizeof(wc);

    wc.style = CS_HREDRAW | CS_VREDRAW;

    wc.lpfnWndProc = WindowProc;

    wc.hInstance = GetModuleHandleW(nullptr);

    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    wc.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&wc))
    {
        const DWORD error = GetLastError();

        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            return HRESULT_FROM_WIN32(error);
        }
    }

    m_ClassRegistered = true;

    return S_OK;
}

void
TransitionOverlayWindow::RefreshVirtualDesktopRect()
{
    m_VirtualDesktopRect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);

    m_VirtualDesktopRect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    m_VirtualDesktopRect.right = m_VirtualDesktopRect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);

    m_VirtualDesktopRect.bottom = m_VirtualDesktopRect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    wprintf(
        L"[Transition] OverlayDesktop "
        L"(%ld,%ld)-(%ld,%ld)\n",
        m_VirtualDesktopRect.left,
        m_VirtualDesktopRect.top,
        m_VirtualDesktopRect.right,
        m_VirtualDesktopRect.bottom);
}

HRESULT
TransitionOverlayWindow::Create()
{
    RETURN_IF_FAILED(RegisterClass());

    RefreshVirtualDesktopRect();

    const int width = m_VirtualDesktopRect.right - m_VirtualDesktopRect.left;

    const int height = m_VirtualDesktopRect.bottom - m_VirtualDesktopRect.top;

    m_Window = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        kWindowClassName,
        L"TransitionOverlay",
        WS_POPUP,
        m_VirtualDesktopRect.left,
        m_VirtualDesktopRect.top,
        width,
        height,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        this);

    if (!m_Window)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    RECT windowRect{};

    GetWindowRect(m_Window, &windowRect);

    wprintf(
        L"[Transition] OverlayWindow "
        L"(%ld,%ld)-(%ld,%ld) "
        L"size=%ldx%ld\n",
        windowRect.left,
        windowRect.top,
        windowRect.right,
        windowRect.bottom,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top);

    SetLayeredWindowAttributes(m_Window, 0, 255, LWA_ALPHA);

    return S_OK;
}

void
TransitionOverlayWindow::Destroy()
{
    if (m_Window)
    {
        DestroyWindow(m_Window);

        m_Window = nullptr;
    }
}

void
TransitionOverlayWindow::Show()
{
    if (!m_Window)
    {
        return;
    }

    ShowWindow(m_Window, SW_SHOWNOACTIVATE);

    UpdateWindow(m_Window);

    RECT clientRect{};

    GetClientRect(m_Window, &clientRect);

    wprintf(
        L"[Transition] OverlayClient "
        L"%ldx%ld\n",
        clientRect.right,
        clientRect.bottom);
}

void
TransitionOverlayWindow::Hide()
{
    if (!m_Window)
    {
        return;
    }

    ShowWindow(m_Window, SW_HIDE);
}

LRESULT CALLBACK
TransitionOverlayWindow::WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    TransitionOverlayWindow *self =
        reinterpret_cast<TransitionOverlayWindow *>(GetWindowLongPtrW(Window, GWLP_USERDATA));

    switch (Message)
    {
    case WM_NCCREATE: {
        auto *createStruct = reinterpret_cast<CREATESTRUCTW *>(LParam);

        SetWindowLongPtrW(Window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));

        return TRUE;
    }

    case WM_DISPLAYCHANGE: {
        if (self)
        {
            self->RefreshVirtualDesktopRect();
        }

        return 0;
    }

    case WM_DPICHANGED: {
        if (LParam)
        {
            const RECT *suggested = reinterpret_cast<const RECT *>(LParam);

            SetWindowPos(
                Window,
                nullptr,
                suggested->left,
                suggested->top,
                suggested->right - suggested->left,
                suggested->bottom - suggested->top,
                SWP_NOACTIVATE | SWP_NOZORDER);
        }

        return 0;
    }

    case WM_ERASEBKGND:

        return 1;

    case WM_NCHITTEST:

        return HTTRANSPARENT;
    }

    return DefWindowProcW(Window, Message, WParam, LParam);
}

} // namespace Transition
