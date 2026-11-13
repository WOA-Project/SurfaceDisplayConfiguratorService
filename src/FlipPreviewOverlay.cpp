// FlipPreviewOverlay.cpp

#include "pch.h"

#include <Windows.h>
#include <dwmapi.h>
#include <math.h>

#include "resource.h"

#include <uxtheme.h>

#include "FlipConfirmationManager.h"

#define IUnknown ::IUnknown

#include <d2d1.h>
#include <dwrite.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_USE_HOSTBACKDROPBRUSH
#    define DWMWA_USE_HOSTBACKDROPBRUSH 17
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#    define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

typedef enum DWM_SYSTEMBACKDROP_TYPE
{
    DWMSBT_AUTO = 0,
    DWMSBT_NONE = 1,
    DWMSBT_MAINWINDOW = 2,
    DWMSBT_TRANSIENTWINDOW = 3,
    DWMSBT_TABBEDWINDOW = 4
} DWM_SYSTEMBACKDROP_TYPE;

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

static HANDLE g_OverlayThread = nullptr;
static DWORD g_OverlayThreadId = 0;
static HWND g_OverlayWindow = nullptr;

static HANDLE g_SurfaceFontResource = nullptr;

static float g_AnimationPhase = 0.0f;

static BYTE g_WindowAlpha = 0;
static bool g_FadingOut = false;

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

    wprintf(L"Font loaded. Count=%lu Handle=%p\n", fontCount, g_SurfaceFontResource);

    return g_SurfaceFontResource != nullptr;
}

struct OverlayCreateParams
{
    RECT Rect;
};

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

    /*BOOL hostBackdrop = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_HOSTBACKDROPBRUSH, &hostBackdrop, sizeof(hostBackdrop));

    DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_TRANSIENTWINDOW;
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));*/
}

static LRESULT CALLBACK
OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        float breathing = (sinf(g_AnimationPhase) + 1.0f) * 0.5f;

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
        DrawTextW(hdc, L"Double tap to switch screens", -1, &textRect, DT_CENTER | DT_SINGLELINE);

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
        g_AnimationPhase += 0.025f;
        if (g_AnimationPhase > 6.2831853f)
        {
            g_AnimationPhase -= 6.2831853f;
        }

        if (g_FadingOut)
        {
            if (g_WindowAlpha > 4)
            {
                float alpha = g_WindowAlpha / 255.0f;
                alpha *= 0.93f;
                g_WindowAlpha = (BYTE)(alpha * 255.0f);
                SetLayeredWindowAttributes(hwnd, 0, g_WindowAlpha, LWA_ALPHA);
            }
            else
            {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }
        }
        else if (g_WindowAlpha < 255)
        {
            g_WindowAlpha = (BYTE)std::min(255, g_WindowAlpha + 10);
            SetLayeredWindowAttributes(hwnd, 0, g_WindowAlpha, LWA_ALPHA);
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
    LoadSurfaceAppFluentFont();
    BufferedPaintInit();

    OverlayCreateParams *Params = (OverlayCreateParams *)Context;

    //
    // Register class
    //
    WNDCLASSW wc{};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"FlipPreviewOverlay";

    RegisterClassW(&wc);

    //
    // Create window
    //
    g_FadingOut = false;
    g_WindowAlpha = 0;

    g_OverlayWindow = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        L"",
        WS_POPUP | WS_VISIBLE,
        Params->Rect.left,
        Params->Rect.top,
        Params->Rect.right - Params->Rect.left,
        Params->Rect.bottom - Params->Rect.top,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    if (g_OverlayWindow)
    {
        SetLayeredWindowAttributes(g_OverlayWindow, 0, 0, LWA_ALPHA);
        ApplyBackdrop(g_OverlayWindow);

        ShowWindow(g_OverlayWindow, SW_SHOW);
        SetTimer(g_OverlayWindow, 1, 16, nullptr);
        InvalidateRect(g_OverlayWindow, nullptr, FALSE);
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

    if (g_SurfaceFontResource)
    {
        RemoveFontMemResourceEx(g_SurfaceFontResource);
        g_SurfaceFontResource = nullptr;
    }

    g_OverlayWindow = nullptr;
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
    if (g_OverlayWindow)
    {
        g_FadingOut = true;
        return;
    }

    if (g_OverlayThread)
    {
        WaitForSingleObject(g_OverlayThread, 3000);
        CloseHandle(g_OverlayThread);
        g_OverlayThread = nullptr;
    }

    if (g_SurfaceFontResource)
    {
        RemoveFontMemResourceEx(g_SurfaceFontResource);
        g_SurfaceFontResource = nullptr;
    }

    g_OverlayThreadId = 0;
}

void
ForceHideFlipPreviewOverlay()
{
    if (g_OverlayWindow)
    {
        PostMessage(g_OverlayWindow, WM_CLOSE, 0, 0);
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

    auto params = new OverlayCreateParams;
    params->Rect = ctx.Rect;
    g_OverlayThread = CreateThread(nullptr, 0, OverlayThreadProc, params, 0, &g_OverlayThreadId);

    return g_OverlayThread ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

bool
IsFlipPreviewOverlayVisible()
{
    return g_OverlayWindow != nullptr;
}