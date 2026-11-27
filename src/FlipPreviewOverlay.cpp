// FlipPreviewOverlay.cpp

#include "pch.h"

#include <Windows.h>
#include <dwmapi.h>
#include <math.h>

#include <algorithm>
#include "resource.h"

#include <d3d11.h>
#include <d2d1_1.h>
#include <dwrite_3.h>
#include <windows.system.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>

#include <dispatcherqueue.h>
#include <windows.ui.composition.interop.h>

#include "FlipConfirmationManager.h"
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace winrt;
using namespace Windows::System;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;
using namespace Windows::Graphics::DirectX;

using namespace D2D1;

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

// static HANDLE g_SurfaceFontResource = nullptr;

struct OverlayInstance
{
    HWND Window = nullptr;
    RECT Rect{};

    bool m_done;
    DispatcherQueueController m_dispatcherQueueController{nullptr};
    Compositor m_compositor{nullptr};
    DesktopWindowTarget m_target{nullptr};
    ContainerVisual m_rootVisual{nullptr};
    ContainerVisual m_hostVisual{nullptr};
    DispatcherQueueTimer m_delayTimer{nullptr};
};

static CRITICAL_SECTION g_OverlayLock;
struct OverlayLockInitializer
{
    OverlayLockInitializer() { InitializeCriticalSection(&g_OverlayLock); }
    ~OverlayLockInitializer() { DeleteCriticalSection(&g_OverlayLock); }
};
static OverlayLockInitializer g_OverlayLockInitializer;

static std::vector<std::shared_ptr<OverlayInstance>> g_Overlays;

struct OverlayCreateParams
{
    std::shared_ptr<OverlayInstance> Instance;
};

static std::tuple<winrt::com_ptr<IDWriteFontCollection1>, std::wstring, winrt::com_ptr<IDWriteInMemoryFontFileLoader>>
LoadCustomFontCollection(IDWriteFactory5 *dwriteFactory)
{
    auto module = GetModuleHandleW(nullptr);
    auto resource = FindResourceW(module, MAKEINTRESOURCEW(IDR_SURFACE_APP_FLUENT_FONT), RT_RCDATA);
    winrt::check_pointer(resource);

    auto resourceData = LoadResource(module, resource);
    winrt::check_pointer(resourceData);

    auto size = SizeofResource(module, resource);
    auto data = LockResource(resourceData);
    winrt::check_pointer(data);

    winrt::com_ptr<IDWriteInMemoryFontFileLoader> inMemoryLoader;
    winrt::check_hresult(dwriteFactory->CreateInMemoryFontFileLoader(inMemoryLoader.put()));
    winrt::check_hresult(dwriteFactory->RegisterFontFileLoader(inMemoryLoader.get()));

    winrt::com_ptr<IDWriteFontFile> fontFile;
    winrt::check_hresult(
        inMemoryLoader->CreateInMemoryFontFileReference(dwriteFactory, data, size, nullptr, fontFile.put()));

    winrt::com_ptr<IDWriteFontSetBuilder1> fontSetBuilder;
    winrt::check_hresult(dwriteFactory->CreateFontSetBuilder(fontSetBuilder.put()));
    winrt::check_hresult(fontSetBuilder->AddFontFile(fontFile.get()));

    winrt::com_ptr<IDWriteFontSet> fontSet;
    winrt::check_hresult(fontSetBuilder->CreateFontSet(fontSet.put()));

    winrt::com_ptr<IDWriteFontCollection1> fontCollection;
    winrt::check_hresult(dwriteFactory->CreateFontCollectionFromFontSet(fontSet.get(), fontCollection.put()));

    winrt::com_ptr<IDWriteFontFamily> family;
    winrt::check_hresult(fontCollection->GetFontFamily(0, family.put()));

    winrt::com_ptr<IDWriteLocalizedStrings> familyNames;
    winrt::check_hresult(family->GetFamilyNames(familyNames.put()));

    UINT32 length = 0;
    winrt::check_hresult(familyNames->GetStringLength(0, &length));

    std::wstring familyName(length, L'\0');
    winrt::check_hresult(familyNames->GetString(0, familyName.data(), length + 1));

    return {fontCollection, familyName, inMemoryLoader};
}

static SpriteVisual
CreateText(
    Compositor const &compositor,
    CompositionGraphicsDevice const &graphicsDevice,
    IDWriteFactory5 *dwriteFactory,
    float width,
    float height)
{
    auto surface = graphicsDevice.CreateDrawingSurface(
        {width, height}, DirectXPixelFormat::B8G8R8A8UIntNormalized, DirectXAlphaMode::Premultiplied);

    auto surfaceInterop = surface.as<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop>();

    POINT offset{};
    winrt::com_ptr<ID2D1DeviceContext> d2d1Context;
    winrt::check_hresult(surfaceInterop->BeginDraw(nullptr, IID_ID2D1DeviceContext, d2d1Context.put_void(), &offset));

    d2d1Context->SetTransform(Matrix3x2F::Translation(static_cast<float>(offset.x), static_cast<float>(offset.y)));
    d2d1Context->SetDpi(96.0f, 96.0f);
    d2d1Context->Clear(ColorF(0, 0.0f));

    winrt::com_ptr<ID2D1SolidColorBrush> textBrush;
    winrt::check_hresult(d2d1Context->CreateSolidColorBrush(ColorF(ColorF::White, 1.0f), textBrush.put()));

    auto [symbolCollection, symbolFamilyName, inMemoryLoader] = LoadCustomFontCollection(dwriteFactory);

    winrt::com_ptr<IDWriteTextFormat> symbolFormat;
    winrt::check_hresult(dwriteFactory->CreateTextFormat(
        symbolFamilyName.c_str(),
        symbolCollection.get(),
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        280.0f,
        L"en-us",
        symbolFormat.put()));
    symbolFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    symbolFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    winrt::com_ptr<IDWriteTextFormat> textFormat;
    winrt::check_hresult(dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        46.0f,
        L"en-us",
        textFormat.put()));
    textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    auto groupHeight = 520.0f;
    auto groupTop = (height - groupHeight) * 0.5f;

    auto symbolRect = RectF(0.0f, groupTop + 10.0f, width, groupTop + 380.0f);
    auto textRect = RectF(0.0f, groupTop + 430.0f, width, groupTop + groupHeight);

    std::wstring symbolText = L"\uF4CE";
    std::wstring labelText = L"Double-tap to switch screens";

    d2d1Context->DrawText(
        symbolText.c_str(), static_cast<UINT32>(symbolText.size()), symbolFormat.get(), symbolRect, textBrush.get());

    d2d1Context->DrawText(
        labelText.c_str(), static_cast<UINT32>(labelText.size()), textFormat.get(), textRect, textBrush.get());

    winrt::check_hresult(surfaceInterop->EndDraw());

    winrt::check_hresult(dwriteFactory->UnregisterFontFileLoader(inMemoryLoader.get()));

    auto brush = compositor.CreateSurfaceBrush(surface);
    brush.Stretch(CompositionStretch::None);

    auto visual = compositor.CreateSpriteVisual();
    visual.Brush(brush);
    visual.Size({width, height});
    visual.AnchorPoint({0.5f, 0.5f});
    visual.RelativeOffsetAdjustment({0.5f, 0.5f, 0.0f});

    return visual;
}

static void
SetupContent(std::shared_ptr<OverlayInstance> instance)
{
    auto composition = reinterpret_cast<SetWindowCompositionAttribute_t>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

    if (!composition)
    {
        return;
    }

    ACCENT_POLICY policy{};
    policy.AccentState = ACCENT_ENABLE_HOSTBACKDROP;

    WINDOWCOMPOSITIONATTRIBDATA data{};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &policy;
    data.cbData = sizeof(policy);

    composition(instance->Window, &data);

    DispatcherQueueOptions options{sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_STA};
    winrt::check_hresult(CreateDispatcherQueueController(
        options,
        reinterpret_cast<ABI::Windows::System::IDispatcherQueueController **>(
            winrt::put_abi(instance->m_dispatcherQueueController))));

    namespace abi = ABI::Windows::UI::Composition::Desktop;
    auto &m_compositor = instance->m_compositor = Compositor();

    auto interop = m_compositor.as<abi::ICompositorDesktopInterop>();

    interop->CreateDesktopWindowTarget(
        instance->Window, true, reinterpret_cast<abi::IDesktopWindowTarget **>(winrt::put_abi(instance->m_target)));

    auto &m_rootVisual = instance->m_rootVisual = m_compositor.CreateContainerVisual();
    m_rootVisual.RelativeSizeAdjustment({1.0f, 1.0f});

    auto &m_hostVisual = instance->m_hostVisual = m_compositor.CreateContainerVisual();
    m_hostVisual.RelativeSizeAdjustment({1.0f, 1.0f});
    m_hostVisual.Opacity(0.0f);

    auto backdropVisual = m_compositor.CreateSpriteVisual();
    backdropVisual.Brush(m_compositor.CreateHostBackdropBrush());
    backdropVisual.RelativeSizeAdjustment({1.0f, 1.0f});

    auto tintVisual = m_compositor.CreateSpriteVisual();
    tintVisual.Brush(m_compositor.CreateColorBrush(ColorHelper::FromArgb(99, 20, 20, 20)));
    tintVisual.RelativeSizeAdjustment({1.0f, 1.0f});

    winrt::com_ptr<ID3D11Device> device;
    auto d3dDriverTypes = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP};
    HRESULT d3d11HR = NULL;

    for (const auto &driver : d3dDriverTypes)
    {
        d3d11HR = D3D11CreateDevice(
            nullptr,
            driver,
            NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            device.put(),
            nullptr,
            nullptr);
        if (SUCCEEDED(d3d11HR))
            break;
    }

    winrt::check_hresult(d3d11HR);

    winrt::com_ptr<ID2D1Device> d2d1Device;
    auto pDXGIDevice = device.as<IDXGIDevice>();
    winrt::check_hresult(D2D1CreateDevice(pDXGIDevice.get(), nullptr, d2d1Device.put()));

    winrt::com_ptr<IDWriteFactory5> dwriteFactory;
    winrt::check_hresult(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown **>(dwriteFactory.put())));

    auto compositorInterop = m_compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();
    CompositionGraphicsDevice graphicsDevice{nullptr};
    winrt::check_hresult(compositorInterop->CreateGraphicsDevice(
        d2d1Device.get(),
        reinterpret_cast<ABI::Windows::UI::Composition::ICompositionGraphicsDevice **>(
            winrt::put_abi(graphicsDevice))));

    RECT rc{};
    GetClientRect(instance->Window, &rc);
    float width = static_cast<float>(std::max<LONG>(1, rc.right - rc.left));
    float height = static_cast<float>(std::max<LONG>(1, rc.bottom - rc.top));

    m_hostVisual.Children().InsertAtTop(backdropVisual);
    m_hostVisual.Children().InsertAtTop(tintVisual);
    m_hostVisual.Children().InsertAtTop(CreateText(m_compositor, graphicsDevice, dwriteFactory.get(), width, height));

    m_rootVisual.Children().InsertAtBottom(m_hostVisual);

    instance->m_target.Root(m_rootVisual);

    auto fadeIn = m_compositor.CreateScalarKeyFrameAnimation();
    fadeIn.InsertKeyFrame(1.0f, 1.0f);
    fadeIn.Duration(std::chrono::milliseconds(1000));

    auto fadeInBatch = m_compositor.CreateScopedBatch(CompositionBatchTypes::Animation);

    m_hostVisual.StartAnimation(L"Opacity", fadeIn);

    fadeInBatch.Completed([instance](auto const &, auto const &) {
        if (instance->m_done)
            return;

        auto dq = instance->m_dispatcherQueueController.DispatcherQueue();

        instance->m_delayTimer = dq.CreateTimer();
        instance->m_delayTimer.Interval(std::chrono::seconds(5));
        instance->m_delayTimer.IsRepeating(false);

        instance->m_delayTimer.Tick([instance](auto const &, auto const &) {
            if (instance->m_done)
                return;

            auto fadeOut = instance->m_compositor.CreateScalarKeyFrameAnimation();
            fadeOut.InsertKeyFrame(1.0f, 0.0f);
            fadeOut.Duration(std::chrono::milliseconds(1000));

            auto fadeOutBatch = instance->m_compositor.CreateScopedBatch(CompositionBatchTypes::Animation);

            instance->m_hostVisual.StartAnimation(L"Opacity", fadeOut);

            fadeOutBatch.Completed(
                [instance](auto const &, auto const &) { PostMessage(instance->Window, WM_CLOSE, 0, 0); });
            fadeOutBatch.End();

        });

        instance->m_delayTimer.Start();
    });
    fadeInBatch.End();
}

static OverlayInstance *
GetInstance(HWND hwnd)
{
    auto params = reinterpret_cast<OverlayCreateParams *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return params ? params->Instance.get() : nullptr;
}

static LRESULT CALLBACK
OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCCREATE: {
        auto cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return TRUE;
    }
    case WM_LBUTTONDBLCLK: {
        auto *Instance = GetInstance(hwnd);
        if (Instance->m_done)
        {
            return 0;
        }
        Instance->m_done = true;

        auto tintVisual = Instance->m_compositor.CreateSpriteVisual();
        tintVisual.Brush(Instance->m_compositor.CreateColorBrush(ColorHelper::FromArgb(255, 0, 0, 0)));
        tintVisual.RelativeSizeAdjustment({1.0f, 1.0f});
        tintVisual.Opacity(0.0);

        Instance->m_rootVisual.Children().InsertAtTop(tintVisual);

        auto fadeIn = Instance->m_compositor.CreateScalarKeyFrameAnimation();
        fadeIn.InsertKeyFrame(1.0f, 1.0f);
        fadeIn.Duration(std::chrono::milliseconds(300));

        auto batch = Instance->m_compositor.CreateScopedBatch(CompositionBatchTypes::Animation);
        tintVisual.StartAnimation(L"Opacity", fadeIn);

        batch.Completed([](auto const &, auto const &) { TogglePendingFlipPreview(); });
        batch.End();
        return 0;
    }
    case WM_DESTROY: {
        auto *Instance = GetInstance(hwnd);

        Instance->m_delayTimer = nullptr;
        Instance->m_hostVisual = nullptr;
        Instance->m_rootVisual = nullptr;
        Instance->m_target = nullptr;
        Instance->m_compositor = nullptr;

        auto shutdownOp = Instance->m_dispatcherQueueController.ShutdownQueueAsync();
        shutdownOp.Completed([Instance](auto const &, auto const &) {
            PostQuitMessage(0);
            Instance->m_dispatcherQueueController = nullptr;
        });
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static DWORD WINAPI
OverlayThreadProc(LPVOID Context)
{
    OverlayCreateParams *Params = (OverlayCreateParams *)Context;
    std::shared_ptr<OverlayInstance> Instance = Params->Instance;

    //
    // Register class
    //
    WNDCLASSW wc{};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"FlipPreviewOverlay";

    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        EnterCriticalSection(&g_OverlayLock);
        auto it = std::find(g_Overlays.begin(), g_Overlays.end(), Instance);
        if (it != g_Overlays.end())
        {
            g_Overlays.erase(it);
        }
        LeaveCriticalSection(&g_OverlayLock);

        delete Params;
        return 0;
    }

    //
    // Create window
    //

    Instance->Window = CreateWindowExW(
        WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"",
        WS_POPUP | WS_VISIBLE,
        Instance->Rect.left,
        Instance->Rect.top,
        Instance->Rect.right - Instance->Rect.left,
        Instance->Rect.bottom - Instance->Rect.top,
        nullptr,
        nullptr,
        wc.hInstance,
        Params);

    SetupContent(Instance);

    //
    // Message loop
    //
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Instance->Window = nullptr;

    EnterCriticalSection(&g_OverlayLock);
    auto it = std::find(g_Overlays.begin(), g_Overlays.end(), Instance);
    if (it != g_Overlays.end())
    {
        g_Overlays.erase(it);
    }
    LeaveCriticalSection(&g_OverlayLock);
    delete Params;
    return 0;
}

static BOOL CALLBACK
FindMonitorByDisplayName(HMONITOR monitor, HDC, LPRECT, LPARAM context)
{
    struct Context
    {
        PCWSTR DisplayName;
        RECT Rect;
        BOOL Found;
    };

    auto ctx = reinterpret_cast<Context *>(context);

    wprintf(L"Looking for: %s\n", ctx->DisplayName);

    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info))
    {
        return TRUE;
    }

    wprintf(L"Monitor found: %s\n", info.szDevice);
    if (_wcsicmp(info.szDevice, ctx->DisplayName) == 0)
    {
        ctx->Rect = info.rcMonitor;
        ctx->Found = TRUE;
        return FALSE;
    }

    return TRUE;
}

void
HideFlipPreviewOverlay()
{
    EnterCriticalSection(&g_OverlayLock);
    auto overlays = g_Overlays;
    LeaveCriticalSection(&g_OverlayLock);
    for (auto overlay : overlays)
    {
        overlay->m_done = true;
        auto fadeIn = overlay->m_compositor.CreateScalarKeyFrameAnimation();
        fadeIn.InsertKeyFrame(1.0f, 1.0f);
        fadeIn.Duration(std::chrono::milliseconds(300));

        auto batch = overlay->m_compositor.CreateScopedBatch(CompositionBatchTypes::Animation);
        overlay->m_rootVisual.StartAnimation(L"Opacity", fadeIn);

        batch.Completed([](auto const &, auto const &) { PostMessage(overlay->Window, WM_CLOSE, 0, 0); });
        batch.End();
    }
}

void
ForceHideFlipPreviewOverlay()
{
    EnterCriticalSection(&g_OverlayLock);
    auto overlays = g_Overlays;
    LeaveCriticalSection(&g_OverlayLock);
    for (auto overlay : overlays)
    {
        if (overlay->Window)
        {
            PostMessage(overlay->Window, WM_CLOSE, 0, 0);
        }
    }
}

HRESULT
ShowFlipPreviewOverlay(PCWSTR DisplayName)
{
    HideFlipPreviewOverlay();

    struct
    {
        PCWSTR DisplayName;
        RECT Rect;
        BOOL Found;
    } ctx = {DisplayName, {}, FALSE};

    EnumDisplayMonitors(nullptr, nullptr, FindMonitorByDisplayName, reinterpret_cast<LPARAM>(&ctx));

    wprintf(L"Found=%d\n", ctx.Found);

    if (!ctx.Found)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    auto overlay = std::make_shared<OverlayInstance>();

    overlay->Rect = ctx.Rect;

    //
    // Register ownership BEFORE starting the thread.
    //
    EnterCriticalSection(&g_OverlayLock);
    g_Overlays.push_back(overlay);
    LeaveCriticalSection(&g_OverlayLock);

    auto params = new OverlayCreateParams();
    params->Instance = overlay;

    DWORD ThreadId = 0;
    HANDLE Thread = CreateThread(nullptr, 0, OverlayThreadProc, params, 0, &ThreadId);

    if (!Thread)
    {
        EnterCriticalSection(&g_OverlayLock);
        auto it = std::find(g_Overlays.begin(), g_Overlays.end(), overlay);
        if (it != g_Overlays.end())
        {
            g_Overlays.erase(it);
        }
        LeaveCriticalSection(&g_OverlayLock);
        delete params;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    CloseHandle(Thread);
    return S_OK;
}

bool
IsFlipPreviewOverlayVisible()
{
    EnterCriticalSection(&g_OverlayLock);
    bool visible = !g_Overlays.empty();
    LeaveCriticalSection(&g_OverlayLock);
    return visible;
}