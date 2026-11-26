#include "pch.h"
#include "Overlay/OverlayWindow.h"

#include <dwmapi.h>
#include <algorithm>
#include <dispatcherqueue.h>
#include <windows.ui.composition.interop.h>

#pragma comment(lib, "CoreMessaging.lib")
#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Msimg32.lib")

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
} // namespace

OverlayWindow::~OverlayWindow()
{
    m_dispatcherQueueController = nullptr;
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
        WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,

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

    if (auto composition = GetCompositionFunction())
    {
        ACCENT_POLICY policy{};

        policy.AccentState = ACCENT_ENABLE_HOSTBACKDROP;

        WINDOWCOMPOSITIONATTRIBDATA data{};

        data.Attrib = WCA_ACCENT_POLICY;

        data.pvData = &policy;

        data.cbData = sizeof(policy);

        composition(m_window, &data);
    }

    DispatcherQueueOptions options{sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_STA};
    winrt::check_hresult(CreateDispatcherQueueController(
        options,
        reinterpret_cast<ABI::Windows::System::IDispatcherQueueController **>(
            winrt::put_abi(m_dispatcherQueueController))));

    namespace abi = ABI::Windows::UI::Composition::Desktop;
    m_compositor = winrt::Windows::UI::Composition::Compositor();

    auto interop = m_compositor.as<abi::ICompositorDesktopInterop>();

    interop->CreateDesktopWindowTarget(
        m_window, true, reinterpret_cast<abi::IDesktopWindowTarget **>(winrt::put_abi(m_target)));

    m_rootVisual = m_compositor.CreateContainerVisual();
    m_rootVisual.RelativeSizeAdjustment({1.0f, 1.0f});

    m_hostVisual = m_compositor.CreateContainerVisual();
    m_hostVisual.Size({100, 100});
    m_hostVisual.Offset({VisualInset, VisualInset, 0.0f});

    auto cornerRadius = 16.0f;
    auto borderThickness = 2.0f;
    auto contentRadius = (std::max)(0.0f, cornerRadius - borderThickness);

    auto clipGeometry = m_compositor.CreateRoundedRectangleGeometry();
    clipGeometry.Offset({borderThickness, borderThickness});
    clipGeometry.CornerRadius({contentRadius, contentRadius});

    auto geometricClip = m_compositor.CreateGeometricClip(clipGeometry);

    auto backdropVisual = m_compositor.CreateSpriteVisual();
    backdropVisual.Brush(m_compositor.CreateHostBackdropBrush());
    backdropVisual.Clip(geometricClip);
    backdropVisual.RelativeSizeAdjustment({1.0f, 1.0f});

    auto tintVisual = m_compositor.CreateSpriteVisual();
    tintVisual.Brush(m_compositor.CreateColorBrush(ColorHelper::FromArgb(24, 80, 80, 80)));
    tintVisual.Clip(geometricClip);
    tintVisual.RelativeSizeAdjustment({1.0f, 1.0f});

    auto halfStroke = borderThickness / 2.0f;
    auto innerRadius = (std::max)(0.0f, cornerRadius - halfStroke);

    auto borderGeometry = m_compositor.CreateRoundedRectangleGeometry();
    borderGeometry.Offset({halfStroke, halfStroke});
    borderGeometry.CornerRadius({innerRadius, innerRadius});

    auto borderShape = m_compositor.CreateSpriteShape(borderGeometry);
    borderShape.StrokeBrush(m_compositor.CreateColorBrush(ColorHelper::FromArgb(60, 0, 0, 0)));
    borderShape.StrokeThickness(borderThickness);

    auto borderVisual = m_compositor.CreateShapeVisual();
    borderVisual.RelativeSizeAdjustment({1.0f, 1.0f});
    borderVisual.Shapes().Append(borderShape);

    auto clipSizeAnim = m_compositor.CreateExpressionAnimation(
        L"Vector2(max(0.0, host.Size.X - inset), max(0.0, host.Size.Y - inset))");
    clipSizeAnim.SetReferenceParameter(L"host", m_hostVisual);
    clipSizeAnim.SetScalarParameter(L"inset", borderThickness * 2.0f);
    clipGeometry.StartAnimation(L"Size", clipSizeAnim);

    auto borderSizeAnim = m_compositor.CreateExpressionAnimation(
        L"Vector2(max(0.0, host.Size.X - thickness), max(0.0, host.Size.Y - thickness))");
    borderSizeAnim.SetReferenceParameter(L"host", m_hostVisual);
    borderSizeAnim.SetScalarParameter(L"thickness", borderThickness);
    borderGeometry.StartAnimation(L"Size", borderSizeAnim);

    m_hostVisual.Children().InsertAtTop(backdropVisual);
    m_hostVisual.Children().InsertAtTop(tintVisual);
    m_hostVisual.Children().InsertAtTop(borderVisual);

    m_rootVisual.Children().InsertAtBottom(m_hostVisual);

    return true;
}

void
OverlayWindow::Show(const RECT &rect, HWND insertAfter)
{
    if (!m_window)
    {
        return;
    }

    const LONG width = std::max<LONG>(1, rect.right - rect.left);

    const LONG height = std::max<LONG>(1, rect.bottom - rect.top);

    const auto inset = VisualInset * 2.0f;

    m_hostVisual.Size({static_cast<float>(width) - inset, static_cast<float>(height) - inset});

    m_target.Root(m_rootVisual);

    SetWindowPos(
        m_window,
        insertAfter,

        rect.left,
        rect.top,

        width,
        height,

        SWP_SHOWWINDOW | SWP_NOACTIVATE);

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

    m_target.Root(nullptr);

    m_visible = false;
}

void
OverlayWindow::Destroy()
{
    if (!m_window)
    {
        return;
    }

    Hide();

    m_hostVisual = nullptr;
    m_rootVisual = nullptr;
    m_target = nullptr;
    m_compositor = nullptr;

    m_dispatcherQueueController.ShutdownQueueAsync();

    DestroyWindow(m_window);
    m_window = nullptr;
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
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}