#include "pch.h"

#include "Transition\TransitionDebugOverlay.h"

namespace Transition {

namespace {

constexpr wchar_t kWindowClassName[] = L"TransitionDebugOverlayWindow";

}

LRESULT CALLBACK
TransitionDebugOverlay::WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    TransitionDebugOverlay *self = reinterpret_cast<TransitionDebugOverlay *>(GetWindowLongPtrW(Window, GWLP_USERDATA));

    switch (Message)
    {
    case WM_NCCREATE: {
        auto *createStruct = reinterpret_cast<CREATESTRUCTW *>(LParam);

        SetWindowLongPtrW(Window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));

        return TRUE;
    }

    case WM_PAINT: {
        if (self)
        {
            self->OnPaint();
            return 0;
        }
        break;
    }

    case WM_ERASEBKGND:
        return TRUE;
    }

    return DefWindowProcW(Window, Message, WParam, LParam);
}

HRESULT
TransitionDebugOverlay::Initialize(HWND ParentWindow)
{
    m_ParentWindow = ParentWindow;

    WNDCLASSEXW wc{};

    wc.cbSize = sizeof(wc);

    wc.lpfnWndProc = WindowProc;

    wc.hInstance = GetModuleHandleW(nullptr);

    wc.lpszClassName = kWindowClassName;

    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassExW(&wc);

    m_Window = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        kWindowClassName,
        L"Transition Debug Overlay",
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1,
        1,
        ParentWindow,
        nullptr,
        GetModuleHandleW(nullptr),
        this);

    if (!m_Window)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    SetLayeredWindowAttributes(m_Window, 0, 255, LWA_ALPHA);

    return S_OK;
}

void
TransitionDebugOverlay::Shutdown()
{
    if (m_Window)
    {
        DestroyWindow(m_Window);

        m_Window = nullptr;
    }

    m_Regions.clear();

    m_StatusText.clear();
}

void
TransitionDebugOverlay::Show()
{
    if (!m_Window)
    {
        return;
    }

    ShowWindow(m_Window, SW_SHOW);

    UpdateWindow(m_Window);
}

void
TransitionDebugOverlay::Hide()
{
    if (!m_Window)
    {
        return;
    }

    ShowWindow(m_Window, SW_HIDE);
}

void
TransitionDebugOverlay::Clear()
{
    m_Regions.clear();

    if (m_Window)
    {
        InvalidateRect(m_Window, nullptr, TRUE);
    }
}

void
TransitionDebugOverlay::AddRegion(const DebugRegion &Region)
{
    m_Regions.push_back(Region);

    if (m_Window)
    {
        InvalidateRect(m_Window, nullptr, FALSE);
    }
}

void
TransitionDebugOverlay::SetStatusText(const std::wstring &Text)
{
    m_StatusText = Text;

    if (m_Window)
    {
        InvalidateRect(m_Window, nullptr, FALSE);
    }
}

void
TransitionDebugOverlay::OnPaint()
{
    PAINTSTRUCT ps{};

    HDC dc = BeginPaint(m_Window, &ps);

    if (!dc)
    {
        return;
    }

    SetBkMode(dc, TRANSPARENT);

    if (!m_StatusText.empty())
    {
        RECT textRect{12, 12, 2000, 100};

        SetTextColor(dc, RGB(255, 255, 0));

        DrawTextW(dc, m_StatusText.c_str(), -1, &textRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
    }

    for (const auto &region : m_Regions)
    {
        HPEN pen = CreatePen(PS_SOLID, 3, region.Color);

        HGDIOBJ oldPen = SelectObject(dc, pen);

        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));

        Rectangle(dc, region.Rect.left, region.Rect.top, region.Rect.right, region.Rect.bottom);

        if (!region.Label.empty())
        {
            RECT labelRect = region.Rect;

            SetTextColor(dc, region.Color);

            DrawTextW(dc, region.Label.c_str(), -1, &labelRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
        }

        SelectObject(dc, oldBrush);

        SelectObject(dc, oldPen);

        DeleteObject(pen);
    }

    EndPaint(m_Window, &ps);
}

} // namespace Transition
