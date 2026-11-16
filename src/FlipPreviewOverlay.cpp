// FlipPreviewOverlay.cpp

#include "pch.h"

#include <Windows.h>
#include <dwmapi.h>
#include <math.h>

#include "resource.h"

#include <uxtheme.h>

#include "FlipConfirmationManager.h"

#pragma comment(lib, "uxtheme.lib")

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

static HANDLE g_SurfaceFontResource = nullptr;

struct OverlayInstance
{
    HWND Window = nullptr;
    float AnimationPhase = 0.0f;
    BYTE WindowAlpha = 0;
    bool FadingOut = false;
    RECT Rect{};
};

static CRITICAL_SECTION g_OverlayLock;
static std::vector<std::shared_ptr<OverlayInstance>> g_Overlays;

struct OverlayLockInitializer
{
    OverlayLockInitializer() { InitializeCriticalSection(&g_OverlayLock); }

    ~OverlayLockInitializer() { DeleteCriticalSection(&g_OverlayLock); }
};
static OverlayLockInitializer g_OverlayLockInitializer;

struct OverlayCreateParams
{
    std::shared_ptr<OverlayInstance> Instance;
};

static bool
LoadSurfaceAppFluentFont()
{
    if (g_SurfaceFontResource)
    {
        return true;
    }

    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(IDR_SURFACE_APP_FLUENT_FONT), RT_RCDATA);
    if (!resource)
    {
        return false;
    }

    HGLOBAL resourceData = LoadResource(module, resource);
    if (!resourceData)
    {
        return false;
    }

    DWORD size = SizeofResource(module, resource);
    void *data = LockResource(resourceData);
    if (!data)
    {
        return false;
    }

    DWORD fontCount = 0;
    g_SurfaceFontResource = AddFontMemResourceEx(data, size, nullptr, &fontCount);
    return g_SurfaceFontResource != nullptr;
}

static void
UnloadSurfaceAppFluentFont()
{
    if (!g_SurfaceFontResource)
    {
        return;
    }

    RemoveFontMemResourceEx(g_SurfaceFontResource);
    g_SurfaceFontResource = nullptr;
}

struct FontInitializer
{
    FontInitializer() { LoadSurfaceAppFluentFont(); }
    ~FontInitializer() { UnloadSurfaceAppFluentFont(); }
};

static FontInitializer g_FontInitializer;

static void
ApplyBackdrop(HWND hwnd)
{
    auto composition = reinterpret_cast<SetWindowCompositionAttribute_t>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

    if (!composition)
    {
        return;
    }

    ACCENT_POLICY policy{};
    policy.AccentState = ACCENT_ENABLE_BLURBEHIND;
    policy.GradientColor = 0x99202020;

    WINDOWCOMPOSITIONATTRIBDATA data{};
    data.Attrib = WCA_ACCENT_POLICY;
    data.pvData = &policy;
    data.cbData = sizeof(policy);

    composition(hwnd, &data);

    MARGINS margins = {-1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
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
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        auto *Instance = GetInstance(hwnd);

        if (!Instance)
        {
            EndPaint(hwnd, &ps);
            return 0;
        }

        float breathing = (sinf(Instance->AnimationPhase) + 1.0f) * 0.5f;

        LONG groupHeight = 520;
        LONG groupTop = (rc.bottom - groupHeight) / 2;

        SetBkMode(hdc, TRANSPARENT);

        //
        // Icon
        //
        BYTE brightness = (BYTE)(90 + breathing * 165);
        SetTextColor(hdc, RGB(brightness, brightness, brightness));
        HFONT iconFont = CreateFontW(
            -280,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH,
            L"Surface App Fluent Icons");
        HFONT oldFont = (HFONT)SelectObject(hdc, iconFont);
        RECT iconRect = {0, groupTop + 10, rc.right, groupTop + 380};
        DrawTextW(hdc, L"\xF4CE", 1, &iconRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        //
        // Text
        //
        SelectObject(hdc, oldFont);
        DeleteObject(iconFont);

        HFONT textFont = CreateFontW(
            -46,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH,
            L"Segoe UI");
        oldFont = (HFONT)SelectObject(hdc, textFont);
        SetTextColor(hdc, RGB(255, 255, 255));
        RECT textRect = {0, groupTop + 430, rc.right, groupTop + groupHeight};
        DrawTextW(hdc, L"Double-tap to switch screens", -1, &textRect, DT_CENTER | DT_SINGLELINE);

        SelectObject(hdc, oldFont);
        DeleteObject(textFont);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDBLCLK: {
        TogglePendingFlipPreview();
        return 0;
    }

    case WM_NCHITTEST: {
        return HTCLIENT;
    }

    case WM_DESTROY: {
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }

    case WM_ERASEBKGND: {
        return 1;
    }

    case WM_TIMER: {
        auto *Instance = GetInstance(hwnd);
        if (!Instance)
        {
            return 0;
        }

        Instance->AnimationPhase += 0.025f;
        if (Instance->AnimationPhase > 6.2831853f)
        {
            Instance->AnimationPhase -= 6.2831853f;
        }

        if (Instance->FadingOut)
        {
            if (Instance->WindowAlpha > 4)
            {
                float alpha = Instance->WindowAlpha / 255.0f;
                alpha *= 0.93f;
                Instance->WindowAlpha = (BYTE)(alpha * 255.0f);
                SetLayeredWindowAttributes(hwnd, 0, Instance->WindowAlpha, LWA_ALPHA);
            }
            else
            {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
        }
        else if (Instance->WindowAlpha < 255)
        {
            Instance->WindowAlpha = (BYTE)std::min(255, Instance->WindowAlpha + 10);
            SetLayeredWindowAttributes(hwnd, 0, Instance->WindowAlpha, LWA_ALPHA);
        }

        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
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

    BufferedPaintInit();

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
    Instance->FadingOut = false;
    Instance->WindowAlpha = 0;

    Instance->Window = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
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

    if (Instance->Window)
    {
        SetLayeredWindowAttributes(Instance->Window, 0, 0, LWA_ALPHA);
        ApplyBackdrop(Instance->Window);

        ShowWindow(Instance->Window, SW_SHOW);
        SetTimer(Instance->Window, 1, 16, nullptr);
        InvalidateRect(Instance->Window, nullptr, FALSE);
    }

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
    BufferedPaintUnInit();
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
        overlay->FadingOut = true;
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