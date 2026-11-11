/*

This code was adapted from Microsoft Power Toys' Fancy Zones Source Code
License is reproduced below:

The MIT License

Copyright (c) Microsoft Corporation. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
#include "pch.h"
#include "ActiveMonitorWindowHandler.h"
#include "TabletPostureManager.h"
#include "VirtualDesktop.h"
#include "WorkAreas.h"
#include <on_thread_executor.h>
#include <tchar.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdi32.lib")
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

static void
SetSpanAppearance(HWND window, bool enabled)
{
    DWM_WINDOW_CORNER_PREFERENCE pref = enabled ? DWMWCP_DONOTROUND : DWMWCP_DEFAULT;

    DwmSetWindowAttribute(window, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
}

enum class WindowPlacementPolicy
{
    FitMonitor,            // Current behavior
    PreserveSize,          // Preserve size if possible, otherwise fit virtual desktop
    SpanMonitors,          // Span the entire virtual desktop
    MaximizeMonitor,       // Maximize on target monitor
    MaximizeAcrossDisplays // fill all work areas
};

constexpr WindowPlacementPolicy g_PlacementPolicy = WindowPlacementPolicy::MaximizeAcrossDisplays;

constexpr bool g_ForceResizeNonResizable = true;

#define MAX_TITLE_LENGTH 255

enum AwarenessLevel
{
    UNAWARE,
    SYSTEM_AWARE,
    PER_MONITOR_AWARE,
    PER_MONITOR_AWARE_V2,
    UNAWARE_GDISCALED
};

constexpr int CUSTOM_POSITIONING_LEFT_TOP_PADDING = 16;
constexpr inline int DEFAULT_DPI = 96;
const wchar_t SplashClassName[] = _T("MsoSplash");
const wchar_t SystemAppsFolder[] = _T("SYSTEMAPPS");
const wchar_t CoreWindow[] = _T("Windows.UI.Core.CoreWindow");
const wchar_t SearchUI[] = _T("SearchUI.exe");
const wchar_t PropertyMovedOnOpening[] = _T("FancyZones_MovedOnOpening");
OnThreadExecutor m_dpiUnawareThread;
std::vector<HWINEVENTHOOK> m_staticWinEventHooks;

static RECT
GetCombinedWorkAreaRect();

static HWND g_SpanOverlay = nullptr;
static HWND g_DragWindow{};
static bool g_Dragging{};
static bool g_SpanPreviewVisible = false;
constexpr int PREVIEW_MARGIN = 120;
constexpr int COMMIT_MARGIN = 120;

enum class SeamOrientation
{
    None,
    Vertical,
    Horizontal
};

struct SeamInfo
{
    SeamOrientation orientation;

    LONG position;

    RECT activationZone;
};

constexpr LONG SEAM_TOLERANCE = 32;

static BOOL CALLBACK
saveDisplayToVector(HMONITOR monitor, HDC /*hdc*/, LPRECT /*rect*/, LPARAM data)
{
    reinterpret_cast<std::vector<HMONITOR> *>(data)->emplace_back(monitor);
    return true;
}

static SeamInfo
GetSeamInfo()
{
    std::vector<HMONITOR> monitors;

    EnumDisplayMonitors(nullptr, nullptr, saveDisplayToVector, (LPARAM)&monitors);

    SeamInfo result{};
    result.orientation = SeamOrientation::None;

    if (monitors.size() != 2)
    {
        return result;
    }

    MONITORINFO mi1{};
    MONITORINFO mi2{};

    mi1.cbSize = sizeof(mi1);
    mi2.cbSize = sizeof(mi2);

    GetMonitorInfo(monitors[0], &mi1);
    GetMonitorInfo(monitors[1], &mi2);

    RECT a = mi1.rcMonitor;
    RECT b = mi2.rcMonitor;

    if (a.left > b.left)
    {
        std::swap(a, b);
    }

    if (abs(a.right - b.left) < SEAM_TOLERANCE || abs(b.right - a.left) < SEAM_TOLERANCE)
    {
        result.orientation = SeamOrientation::Vertical;

        result.position = abs(a.right - b.left) < SEAM_TOLERANCE ? a.right : b.right;

        result.activationZone = {
            result.position - PREVIEW_MARGIN,
            min(a.top, b.top),

            result.position + PREVIEW_MARGIN,
            max(a.bottom, b.bottom)};
    }
    else if (abs(a.bottom - b.top) < SEAM_TOLERANCE || abs(b.bottom - a.top) < SEAM_TOLERANCE)
    {
        result.orientation = SeamOrientation::Horizontal;

        result.position = abs(a.bottom - b.top) < SEAM_TOLERANCE ? a.bottom : b.bottom;

        result.activationZone = {
            min(a.left, b.left),

            result.position - PREVIEW_MARGIN,

            max(a.right, b.right),

            result.position + PREVIEW_MARGIN};
    }

    printf(
        "MON1 monitor=(%ld,%ld,%ld,%ld) work=(%ld,%ld,%ld,%ld)\n",
        mi1.rcMonitor.left,
        mi1.rcMonitor.top,
        mi1.rcMonitor.right,
        mi1.rcMonitor.bottom,
        mi1.rcWork.left,
        mi1.rcWork.top,
        mi1.rcWork.right,
        mi1.rcWork.bottom);

    printf(
        "MON2 monitor=(%ld,%ld,%ld,%ld) work=(%ld,%ld,%ld,%ld)\n",
        mi2.rcMonitor.left,
        mi2.rcMonitor.top,
        mi2.rcMonitor.right,
        mi2.rcMonitor.bottom,
        mi2.rcWork.left,
        mi2.rcWork.top,
        mi2.rcWork.right,
        mi2.rcWork.bottom);

    printf("SEAM pos=%ld orientation=%d\n", result.position, (int)result.orientation);

    return result;
}

static bool
allMonitorsHaveSameDpiScaling()
{
    std::vector<HMONITOR> monitors;
    EnumDisplayMonitors(NULL, NULL, saveDisplayToVector, reinterpret_cast<LPARAM>(&monitors));

    if (monitors.size() < 2)
    {
        return true;
    }

    UINT firstMonitorDpiX;
    UINT firstMonitorDpiY;

    if (S_OK != GetDpiForMonitor(monitors[0], MDT_EFFECTIVE_DPI, &firstMonitorDpiX, &firstMonitorDpiY))
    {
        return false;
    }

    for (int i = 1; i < monitors.size(); i++)
    {
        UINT iteratedMonitorDpiX;
        UINT iteratedMonitorDpiY;

        if (S_OK != GetDpiForMonitor(monitors[i], MDT_EFFECTIVE_DPI, &iteratedMonitorDpiX, &iteratedMonitorDpiY) ||
            iteratedMonitorDpiX != firstMonitorDpiX)
        {
            return false;
        }
    }

    return true;
}

static AwarenessLevel
GetAwarenessLevel(DPI_AWARENESS_CONTEXT system_returned_value)
{
    const std::array levels{
        DPI_AWARENESS_CONTEXT_UNAWARE,
        DPI_AWARENESS_CONTEXT_SYSTEM_AWARE,
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE,
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2,
        DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED};
    for (size_t i = 0; i < size(levels); ++i)
    {
        if (AreDpiAwarenessContextsEqual(levels[i], system_returned_value))
        {
            return static_cast<AwarenessLevel>(i);
        }
    }
    return AwarenessLevel::UNAWARE;
}

static void
ScreenToWorkAreaCoords(HWND window, RECT &rect)
{
    // First, find the correct monitor. The monitor cannot be found using the given rect itself, we must first
    // translate it to relative workspace coordinates.
    HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEX monitorInfo{sizeof(MONITORINFOEX)};
    GetMonitorInfo(monitor, &monitorInfo);

    auto xOffset = monitorInfo.rcWork.left - monitorInfo.rcMonitor.left;
    auto yOffset = monitorInfo.rcWork.top - monitorInfo.rcMonitor.top;

    auto referenceRect = rect;

    referenceRect.left -= xOffset;
    referenceRect.right -= xOffset;
    referenceRect.top -= yOffset;
    referenceRect.bottom -= yOffset;

    // Now, this rect should be used to determine the monitor and thus taskbar size. This fixes
    // scenarios where the zone lies approximately between two monitors, and the taskbar is on the left.
    monitor = MonitorFromRect(&referenceRect, MONITOR_DEFAULTTOPRIMARY);
    GetMonitorInfo(monitor, &monitorInfo);

    xOffset = monitorInfo.rcWork.left - monitorInfo.rcMonitor.left;
    yOffset = monitorInfo.rcWork.top - monitorInfo.rcMonitor.top;

    rect.left -= xOffset;
    rect.right -= xOffset;
    rect.top -= yOffset;
    rect.bottom -= yOffset;

    const auto level = GetAwarenessLevel(GetWindowDpiAwarenessContext(window));
    const bool accountForUnawareness = level < PER_MONITOR_AWARE;

    if (accountForUnawareness && !allMonitorsHaveSameDpiScaling())
    {
        rect.left = max(monitorInfo.rcMonitor.left, rect.left);
        rect.right = min(monitorInfo.rcMonitor.right - xOffset, rect.right);
        rect.top = max(monitorInfo.rcMonitor.top, rect.top);
        rect.bottom = min(monitorInfo.rcMonitor.bottom - yOffset, rect.bottom);
    }
}

struct SpanState
{
    WINDOWPLACEMENT placement{};
    bool active{};
};

static std::unordered_map<HWND, SpanState> g_SpanStates;

static void
RestoreSpannedWindow(HWND window)
{
    auto it = g_SpanStates.find(window);

    if (it == g_SpanStates.end())
    {
        return;
    }

    SetSpanAppearance(window, false);

    DwmFlush();

    WINDOWPLACEMENT placement = it->second.placement;

    placement.length = sizeof(WINDOWPLACEMENT);

    RECT current{};
    GetWindowRect(window, &current);

    POINT cursor{};
    GetCursorPos(&cursor);

    RECT restore = placement.rcNormalPosition;

    const int restoreWidth = restore.right - restore.left;

    const int restoreHeight = restore.bottom - restore.top;

    const int currentWidth = current.right - current.left;

    //
    // Preserve where the user grabbed the title bar.
    //
    double grabRatio = 0.5;

    if (currentWidth > 0)
    {
        grabRatio = double(cursor.x - current.left) / double(currentWidth);

        grabRatio = max(0.0, min(1.0, grabRatio));
    }

    RECT target{};

    target.left = cursor.x - LONG(grabRatio * restoreWidth);

    //
    // Mimic Windows maximize-drag behavior.
    //
    const int cursorOffsetY = cursor.y - current.top;

    const int titleBarOffsetY = min(max(cursorOffsetY, 16), 32);

    target.top = cursor.y - titleBarOffsetY;

    target.right = target.left + restoreWidth;

    target.bottom = target.top + restoreHeight;

    printf(
        "RESTORE size=%dx%d "
        "grab=%f "
        "cursor=(%ld,%ld) "
        "target=(%ld,%ld,%ld,%ld)\n",
        restoreWidth,
        restoreHeight,
        grabRatio,
        cursor.x,
        cursor.y,
        target.left,
        target.top,
        target.right,
        target.bottom);

    //
    // Update the normal position but use the
    // cursor-derived location.
    //
    placement.rcNormalPosition = target;

    placement.showCmd = SW_RESTORE;

    SetWindowPlacement(window, &placement);

    ShowWindow(window, SW_RESTORE);

    SetWindowPos(
        window,
        nullptr,
        target.left,
        target.top,
        restoreWidth,
        restoreHeight,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    RemoveProp(window, L"SDC_SPANNED");

    g_SpanStates.erase(it);
}

static void
SpanOrCenterWindow(HWND window)
{
    RECT before{};

    if (!GetWindowRect(window, &before))
    {
        return;
    }

    RECT span = GetCombinedWorkAreaRect();

    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);

    GetWindowPlacement(window, &placement);

    g_SpanStates[window] = {placement, true};

    ShowWindow(window, SW_RESTORE);

    //
    // Make the window look more like a real maximized
    // Windows 11 window.
    //
    SetSpanAppearance(window, true);

    const int borderX = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

    const int borderY = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

    printf("SPAN BORDER x=%d y=%d\n", borderX, borderY);

    SetWindowPos(
        window,
        nullptr,
        span.left - borderX,
        span.top - 2,
        (span.right - span.left) + borderX * 2,
        (span.bottom - span.top) + borderY + 2,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    const int beforeWidth = before.right - before.left;

    const int beforeHeight = before.bottom - before.top;

    RECT actual{};

    for (int i = 0; i < 10; ++i)
    {
        Sleep(100);

        if (!IsWindow(window))
        {
            return;
        }

        GetWindowRect(window, &actual);

        const int currentWidth = actual.right - actual.left;

        const int currentHeight = actual.bottom - actual.top;

        if (currentWidth > beforeWidth + 100 || currentHeight > beforeHeight + 100)
        {
            break;
        }
    }

    const int actualWidth = actual.right - actual.left;

    const int actualHeight = actual.bottom - actual.top;

    const bool resizeIgnored = actualWidth < beforeWidth * 3 / 2 && actualHeight < beforeHeight * 3 / 2;

    printf(
        "SPAN before=%dx%d actual=%dx%d ignored=%d\n",
        beforeWidth,
        beforeHeight,
        actualWidth,
        actualHeight,
        resizeIgnored);

    if (resizeIgnored)
    {
        const int spanWidth = span.right - span.left;

        const int spanHeight = span.bottom - span.top;

        const int x = span.left + (spanWidth - actualWidth) / 2;

        const int y = span.top + (spanHeight - actualHeight) / 2;

        SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
    }

    SetProp(window, L"SDC_SPANNED", (HANDLE)1);
}

static RECT
GetCombinedWorkAreaRect()
{
    RECT combined = {LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN};

    EnumDisplayMonitors(
        nullptr,
        nullptr,
        [](HMONITOR monitor, HDC, LPRECT, LPARAM param) -> BOOL {
            auto rect = reinterpret_cast<RECT *>(param);

            MONITORINFO mi{};
            mi.cbSize = sizeof(mi);

            GetMonitorInfo(monitor, &mi);

            rect->left = min(rect->left, mi.rcWork.left);

            rect->top = min(rect->top, mi.rcWork.top);

            rect->right = max(rect->right, mi.rcWork.right);

            rect->bottom = max(rect->bottom, mi.rcWork.bottom);

            return TRUE;
        },
        reinterpret_cast<LPARAM>(&combined));

    return combined;
}

static void
SizeWindowToRect(HWND window, RECT rect, bool translateToWorkArea = true) noexcept
{
    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);

    if (!GetWindowPlacement(window, &placement))
    {
        return;
    }

    //
    // Wait if the app is still starting minimized.
    //
    for (int i = 0; i < 5 && placement.showCmd == SW_SHOWMINIMIZED; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        GetWindowPlacement(window, &placement);
    }

    const bool wasMaximized = IsZoomed(window) || placement.showCmd == SW_SHOWMAXIMIZED;

    if (translateToWorkArea)
    {
        ScreenToWorkAreaCoords(window, rect);
    }

    //
    // Update the restore rectangle.
    //
    placement.rcNormalPosition = rect;
    placement.flags |= WPF_ASYNCWINDOWPLACEMENT;

    //
    // Preserve current window state.
    //
    if (!IsWindowVisible(window))
    {
        placement.showCmd = SW_HIDE;
    }
    else if (wasMaximized)
    {
        placement.showCmd = SW_SHOWMAXIMIZED;
    }
    else if (placement.showCmd != SW_SHOWMINIMIZED && placement.showCmd != SW_MINIMIZE)
    {
        placement.showCmd = SW_RESTORE;
    }

    if (!SetWindowPlacement(window, &placement))
    {
        return;
    }

    //
    // Some applications ignore the first placement.
    //
    SetWindowPlacement(window, &placement);

    //
    // Force re-maximization on the destination monitor.
    //
    if (wasMaximized)
    {
        ShowWindow(window, SW_MAXIMIZE);
    }
}

inline static int
RectWidth(const RECT &rect)
{
    return rect.right - rect.left;
}

inline static int
RectHeight(const RECT &rect)
{
    return rect.bottom - rect.top;
}

static RECT
GetVirtualDesktopRect()
{
    RECT r;

    r.left = GetSystemMetrics(SM_XVIRTUALSCREEN);

    r.top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    r.right = r.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);

    r.bottom = r.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    return r;
}

static RECT
ConstrainToVirtualDesktop(const RECT &windowRect)
{
    RECT result = windowRect;

    RECT virtualRect = GetVirtualDesktopRect();

    const int virtualWidth = RectWidth(virtualRect);

    const int virtualHeight = RectHeight(virtualRect);

    int width = RectWidth(result);

    int height = RectHeight(result);

    //
    // If the window is larger than the
    // entire virtual desktop, shrink it.
    //
    if (width > virtualWidth)
    {
        result.right = result.left + virtualWidth;

        width = virtualWidth;
    }

    if (height > virtualHeight)
    {
        result.bottom = result.top + virtualHeight;

        height = virtualHeight;
    }

    //
    // Ensure fully visible.
    //
    if (result.left < virtualRect.left)
    {
        OffsetRect(&result, virtualRect.left - result.left, 0);
    }

    if (result.right > virtualRect.right)
    {
        OffsetRect(&result, virtualRect.right - result.right, 0);
    }

    if (result.top < virtualRect.top)
    {
        OffsetRect(&result, 0, virtualRect.top - result.top);
    }

    if (result.bottom > virtualRect.bottom)
    {
        OffsetRect(&result, 0, virtualRect.bottom - result.bottom);
    }

    return result;
}

static RECT
ConstrainToMonitor(HWND hwnd, const RECT &windowRect, const RECT &monitorRect)
{
    RECT result = windowRect;

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);

    const bool canResize = g_ForceResizeNonResizable || ((style & WS_THICKFRAME) != 0);

    const int monitorWidth = RectWidth(monitorRect);

    const int monitorHeight = RectHeight(monitorRect);

    const int width = RectWidth(result);

    const int height = RectHeight(result);

    if ((width > monitorWidth || height > monitorHeight))
    {
        char className[256]{};

        GetClassNameA(hwnd, className, sizeof(className));

        printf(
            "OVERSIZE class=%s style=%p thick=%d max=%d "
            "monitor=(%d,%d) window=(%d,%d)\n",
            className,
            (void *)style,
            !!(style & WS_THICKFRAME),
            !!(style & WS_MAXIMIZEBOX),
            monitorWidth,
            monitorHeight,
            width,
            height);

        printf(
            "RESIZE monitor=(%d,%d) "
            "window=(%d,%d)\n",
            monitorWidth,
            monitorHeight,
            width,
            height);
    }

    if (canResize)
    {
        if (width > monitorWidth)
        {
            result.right = result.left + monitorWidth;
        }

        if (height > monitorHeight)
        {
            result.bottom = result.top + monitorHeight;
        }
    }

    //
    // Horizontal
    //
    if (RectWidth(result) <= monitorWidth)
    {
        if (result.left < monitorRect.left)
        {
            OffsetRect(&result, monitorRect.left - result.left, 0);
        }

        if (result.right > monitorRect.right)
        {
            OffsetRect(&result, monitorRect.right - result.right, 0);
        }
    }

    //
    // Vertical
    //
    if (RectHeight(result) <= monitorHeight)
    {
        if (result.top < monitorRect.top)
        {
            OffsetRect(&result, 0, monitorRect.top - result.top);
        }

        if (result.bottom > monitorRect.bottom)
        {
            OffsetRect(&result, 0, monitorRect.bottom - result.bottom);
        }
    }

    return result;
}

static RECT
FitOnScreen(const RECT &windowRect, const RECT &originMonitorRect, const RECT &destMonitorRect)
{
    // New window position on active monitor. If window fits the screen, this will be final position.
    int left = destMonitorRect.left + (windowRect.left - originMonitorRect.left);
    int top = destMonitorRect.top + (windowRect.top - originMonitorRect.top);
    int W = RectWidth(windowRect);
    int H = RectHeight(windowRect);

    if (left < destMonitorRect.left)
    {
        left = destMonitorRect.left + CUSTOM_POSITIONING_LEFT_TOP_PADDING;
    }

    if (left + W > destMonitorRect.right)
    {
        left = destMonitorRect.right - W - CUSTOM_POSITIONING_LEFT_TOP_PADDING;
    }

    if (top < destMonitorRect.top)
    {
        top = destMonitorRect.top + CUSTOM_POSITIONING_LEFT_TOP_PADDING;
    }

    if (top + H > destMonitorRect.bottom)
    {
        top = destMonitorRect.bottom - H - CUSTOM_POSITIONING_LEFT_TOP_PADDING;
    }

    return {left, top, left + W, top + H};
}

static void
OpenWindowOnActiveMonitor(HWND window, HMONITOR monitor) noexcept
{
    // By default Windows opens new window on primary monitor.
    // Try to preserve window width and height, adjust top-left corner if needed.
    HMONITOR origin = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFOEX originInfo{};
    originInfo.cbSize = sizeof(originInfo);

    MONITORINFOEX targetInfo{};
    targetInfo.cbSize = sizeof(targetInfo);

    GetMonitorInfo(origin, &originInfo);
    GetMonitorInfo(monitor, &targetInfo);

    char buf[512];

    sprintf_s(
        buf,
        "MOVE origin=%p (%ld,%ld,%ld,%ld) "
        "target=%p (%ld,%ld,%ld,%ld)\n",
        origin,
        originInfo.rcMonitor.left,
        originInfo.rcMonitor.top,
        originInfo.rcMonitor.right,
        originInfo.rcMonitor.bottom,
        monitor,
        targetInfo.rcMonitor.left,
        targetInfo.rcMonitor.top,
        targetInfo.rcMonitor.right,
        targetInfo.rcMonitor.bottom);

    printf(buf);

    //
    // Force maximize on selected monitor.
    //
    if (g_PlacementPolicy == WindowPlacementPolicy::MaximizeMonitor)
    {
        WINDOWPLACEMENT placement{};
        placement.length = sizeof(placement);

        if (GetWindowPlacement(window, &placement))
        {
            RECT restore{
                targetInfo.rcWork.left + 100,
                targetInfo.rcWork.top + 100,
                targetInfo.rcWork.left + 1100,
                targetInfo.rcWork.top + 800};

            placement.rcNormalPosition = restore;

            SetWindowPlacement(window, &placement);

            ShowWindow(window, SW_RESTORE);

            ShowWindow(window, SW_MAXIMIZE);
        }

        return;
    }

    if (origin == monitor)
    {
        WINDOWPLACEMENT placement{};
        placement.length = sizeof(placement);

        if (GetWindowPlacement(window, &placement))
        {
            RECT result = placement.rcNormalPosition;

            switch (g_PlacementPolicy)
            {
            case WindowPlacementPolicy::FitMonitor: {
                result = ConstrainToMonitor(window, result, targetInfo.rcWork);
                break;
            }

            case WindowPlacementPolicy::PreserveSize: {
                result = ConstrainToVirtualDesktop(result);
                break;
            }

            case WindowPlacementPolicy::SpanMonitors: {
                result = GetVirtualDesktopRect();
                break;
            }

            case WindowPlacementPolicy::MaximizeMonitor: {
                ShowWindow(window, SW_MAXIMIZE);
                return;
            }
            case WindowPlacementPolicy::MaximizeAcrossDisplays: {
                SpanOrCenterWindow(window);

                return;
            }
            }

            if (!EqualRect(&result, &placement.rcNormalPosition))
            {
                SizeWindowToRect(window, result);
            }
        }

        return;
    }

    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);

    if (GetWindowPlacement(window, &placement))
    {
        MONITORINFOEX originMi;
        originMi.cbSize = sizeof(originMi);

        MONITORINFOEX destMi;
        destMi.cbSize = sizeof(destMi);

        if (GetMonitorInfo(origin, &originMi) && GetMonitorInfo(monitor, &destMi))
        {
            const bool isMaximized = IsZoomed(window) || placement.showCmd == SW_SHOWMAXIMIZED;
            if (isMaximized)
            {
                if (g_PlacementPolicy == WindowPlacementPolicy::MaximizeAcrossDisplays)
                {
                    SpanOrCenterWindow(window);

                    return;
                }

                MONITORINFOEX target{};
                target.cbSize = sizeof(target);

                if (!GetMonitorInfo(monitor, &target))
                {
                    return;
                }

                RECT restore = placement.rcNormalPosition;

                restore.left = target.rcWork.left + 100;

                restore.top = target.rcWork.top + 100;

                restore.right = restore.left + RectWidth(placement.rcNormalPosition);

                restore.bottom = restore.top + RectHeight(placement.rcNormalPosition);

                placement.rcNormalPosition = restore;

                SetWindowPlacement(window, &placement);

                ShowWindow(window, SW_RESTORE);

                ShowWindow(window, SW_MAXIMIZE);

                return;
            }
            else
            {
                RECT newPosition = FitOnScreen(placement.rcNormalPosition, originMi.rcWork, destMi.rcWork);

                switch (g_PlacementPolicy)
                {
                case WindowPlacementPolicy::FitMonitor: {
                    newPosition = ConstrainToMonitor(window, newPosition, destMi.rcWork);
                    break;
                }

                case WindowPlacementPolicy::PreserveSize: {
                    newPosition = ConstrainToVirtualDesktop(newPosition);
                    break;
                }

                case WindowPlacementPolicy::SpanMonitors: {
                    newPosition = GetVirtualDesktopRect();
                    break;
                }

                case WindowPlacementPolicy::MaximizeMonitor: {
                    //
                    // Already handled above.
                    //
                    break;
                }
                case WindowPlacementPolicy::MaximizeAcrossDisplays: {
                    SpanOrCenterWindow(window);

                    return;
                }
                }

                SizeWindowToRect(window, newPosition, false);
            }
        }
    }
}

static bool
IsSplashScreen(HWND window)
{
    wchar_t className[MAX_PATH];
    if (GetClassName(window, className, MAX_PATH) == 0)
    {
        return false;
    }

    return wcscmp(SplashClassName, className) == 0;
}

static bool
IsStandardWindow(HWND window)
{
    // True if from the styles the window looks like a standard window

    if (GetAncestor(window, GA_ROOT) != window)
    {
        return false;
    }

    auto style = GetWindowLong(window, GWL_STYLE);
    auto exStyle = GetWindowLong(window, GWL_EXSTYLE);

    char className[256]{};

    GetClassNameA(window, className, sizeof(className));

    printf(
        "STANDARD class=%s hwnd=%p "
        "style=%08X ex=%08X "
        "root=%d visible=%d tool=%d\n",
        className,
        window,
        style,
        exStyle,
        GetAncestor(window, GA_ROOT) == window,
        !!(style & WS_VISIBLE),
        !!(exStyle & WS_EX_TOOLWINDOW));

    bool isToolWindow = (exStyle & WS_EX_TOOLWINDOW) == WS_EX_TOOLWINDOW;
    bool isVisible = (style & WS_VISIBLE) == WS_VISIBLE;
    if (isToolWindow || !isVisible)
    {
        return false;
    }

    return true;
}

static bool
IsPopupWindow(HWND window) noexcept
{
    auto style = GetWindowLong(window, GWL_STYLE);
    return ((style & WS_POPUP) == WS_POPUP);
}

static bool
HasThickFrame(HWND window) noexcept
{
    auto style = GetWindowLong(window, GWL_STYLE);
    return ((style & WS_THICKFRAME) == WS_THICKFRAME);
}

static bool
HasVisibleOwner(HWND window) noexcept
{
    auto owner = GetWindow(window, GW_OWNER);
    if (owner == nullptr)
    {
        return false; // There is no owner at all
    }
    if (!IsWindowVisible(owner))
    {
        return false; // Owner is invisible
    }
    RECT rect;
    if (!GetWindowRect(owner, &rect))
    {
        return true; // Could not get the rect, return true (and filter out the window) just in case
    }
    // It is enough that the window is zero-sized in one dimension only.
    return rect.top != rect.bottom && rect.left != rect.right;
}

inline static bool
find_folder_in_path(const std::wstring &where, const std::vector<std::wstring> &what)
{
    for (const auto &row : what)
    {
        const auto pos = where.rfind(row);
        if (pos != std::wstring::npos)
        {
            return true;
        }
    }
    return false;
}

// Check if window is part of the shell or the taskbar.
inline static bool
is_system_window(HWND hwnd, const char *class_name)
{
    // We compare the HWND against HWND of the desktop and shell windows,
    // we also filter out some window class names know to belong to the taskbar.
    constexpr std::array system_classes = {
        "SysListView32", "WorkerW", "Shell_TrayWnd", "Shell_SecondaryTrayWnd", "Progman"};
    const std::array system_hwnds = {GetDesktopWindow(), GetShellWindow()};
    for (auto system_hwnd : system_hwnds)
    {
        if (hwnd == system_hwnd)
        {
            return true;
        }
    }
    for (const auto system_class : system_classes)
    {
        if (!strcmp(system_class, class_name))
        {
            return true;
        }
    }
    return false;
}

// Checks if a process path is included in a list of strings.
inline static bool
find_app_name_in_path(const std::wstring &where, const std::vector<std::wstring> &what)
{
    for (const auto &row : what)
    {
        const auto pos = where.rfind(row);
        const auto last_slash = where.rfind('\\');
        // Check that row occurs in where, and its last occurrence contains in itself the first character after the last
        // backslash.
        if (pos != std::wstring::npos && pos <= last_slash + 1 && pos + row.length() > last_slash)
        {
            return true;
        }
    }
    return false;
}

inline static bool
check_excluded_app_with_title(
    const HWND &hwnd,
    std::wstring &processPath,
    const std::vector<std::wstring> &excludedApps)
{
    WCHAR title[MAX_TITLE_LENGTH];
    int len = GetWindowText(hwnd, title, MAX_TITLE_LENGTH);
    if (len <= 0)
    {
        return false;
    }

    std::wstring titleStr(title);
    auto lastBackslashPos = processPath.find_last_of(L'\\');
    if (lastBackslashPos != std::wstring::npos)
    {
        processPath = processPath.substr(0, lastBackslashPos + 1); // retain up to the last backslash
        processPath.append(titleStr);                              // append the title
    }
    CharUpperBuff(processPath.data(), static_cast<DWORD>(processPath.length()));
    return find_app_name_in_path(processPath, excludedApps);
}

inline static bool
check_excluded_app(const HWND &hwnd, std::wstring &processPath, const std::vector<std::wstring> &excludedApps)
{
    bool res = find_app_name_in_path(processPath, excludedApps);

    if (!res)
    {
        res = check_excluded_app_with_title(hwnd, processPath, excludedApps);
    }

    return res;
}

static bool
IsExcludedByDefault(const HWND &hwnd, std::wstring &processPath) noexcept
{
    static std::vector<std::wstring> defaultExcludedFolders = {SystemAppsFolder};
    if (find_folder_in_path(processPath, defaultExcludedFolders))
    {
        return true;
    }

    std::array<char, 256> class_name;
    GetClassNameA(hwnd, class_name.data(), static_cast<int>(class_name.size()));
    if (is_system_window(hwnd, class_name.data()))
    {
        return true;
    }

    static std::vector<std::wstring> defaultExcludedApps = {CoreWindow, SearchUI};
    return (check_excluded_app(hwnd, processPath, defaultExcludedApps));
}

// Get the executable path or module name for modern apps
inline static std::wstring
get_process_path(DWORD pid) noexcept
{
    auto process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    std::wstring name;
    if (process != NULL)
    {
        name.resize(MAX_PATH);
        DWORD name_length = static_cast<DWORD>(name.length());
        if (QueryFullProcessImageName(process, 0, name.data(), &name_length) == 0)
        {
            name_length = 0;
        }
        name.resize(name_length);
        CloseHandle(process);
    }
    return name;
}

// Get the executable path or module name for modern apps
inline static std::wstring
get_process_path(HWND window) noexcept
{
    const static std::wstring app_frame_host = _T("ApplicationFrameHost.exe");

    DWORD pid{};
    GetWindowThreadProcessId(window, &pid);
    auto name = get_process_path(pid);

    if (name.length() >= app_frame_host.length() &&
        name.compare(name.length() - app_frame_host.length(), app_frame_host.length(), app_frame_host) == 0)
    {
        // It is a UWP app. We will enumerate the windows and look for one created
        // by something with a different PID
        DWORD new_pid = pid;

        EnumChildWindows(
            window,
            [](HWND hwnd, LPARAM param) -> BOOL {
                auto new_pid_ptr = reinterpret_cast<DWORD *>(param);
                DWORD pid;
                GetWindowThreadProcessId(hwnd, &pid);
                if (pid != *new_pid_ptr)
                {
                    *new_pid_ptr = pid;
                    return FALSE;
                }
                else
                {
                    return TRUE;
                }
            },
            reinterpret_cast<LPARAM>(&new_pid));

        // If we have a new pid, get the new name.
        if (new_pid != pid)
        {
            return get_process_path(new_pid);
        }
    }

    return name;
}

inline static std::wstring
get_process_path_waiting_uwp(HWND window)
{
    const static std::wstring appFrameHost = _T("ApplicationFrameHost.exe");

    int attempt = 0;
    auto processPath = get_process_path(window);

    while (++attempt < 30 && processPath.length() >= appFrameHost.length() &&
           processPath.compare(processPath.length() - appFrameHost.length(), appFrameHost.length(), appFrameHost) == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        processPath = get_process_path(window);
    }

    return processPath;
}

static bool
IsExcluded(HWND window)
{
    std::wstring processPath = get_process_path_waiting_uwp(window);
    CharUpperBuff(const_cast<std::wstring &>(processPath).data(), static_cast<DWORD>(processPath.length()));

    if (IsExcludedByDefault(window, processPath))
    {
        return true;
    }

    return false;
}

static bool
IsProcessable(HWND window) noexcept
{
    char className[256]{};

    GetClassNameA(window, className, sizeof(className));

    printf("PROCESSABLE CHECK hwnd=%p class=%s\n", window, className);

    if (!_stricmp(className, "XamlExplorerHostIslandWindow"))
    {
        printf("REJECT XamlExplorerHostIslandWindow\n");
        return false;
    }

    const bool isSplashScreen = IsSplashScreen(window);
    if (isSplashScreen)
    {
        printf("REJECT splash\n");
        return false;
    }

    const bool windowMinimized = IsIconic(window);
    if (windowMinimized)
    {
        printf("REJECT minimized\n");
        return false;
    }

    const bool standard = IsStandardWindow(window);
    if (!standard)
    {
        printf("REJECT !standard\n");
        return false;
    }

    // popup could be the window we don't want to snap: start menu, notification popup, tray window, etc.
    // also, popup could be the windows we want to snap disregarding the "allowSnapPopupWindows" setting, e.g. Telegram
    bool isPopup = IsPopupWindow(window);
    bool hasThickFrame = HasThickFrame(window);
    if (isPopup && !hasThickFrame && GetWindow(window, GW_OWNER) != nullptr)
    {
        printf("REJECT isPopup && !hasThickFrame\n");
        return false;
    }

    // allow child windows
    auto hasOwner = HasVisibleOwner(window);
    if (hasOwner)
    {
        printf("REJECT hasOwner\n");
        return false;
    }

    if (IsExcluded(window))
    {
        printf("REJECT IsExcluded\n");
        return false;
    }

    // Switch between virtual desktops results with posting same windows messages that also indicate
    // creation of new window. We need to check if window being processed is on currently active desktop.
    if (!VirtualDesktop::instance().IsWindowOnCurrentDesktop(window))
    {
        printf("REJECT !IsWindowOnCurrentDesktop\n");
        return false;
    }

    printf("ACCEPT\n");
    return true;
}

static bool
IsIgnoredForegroundWindow(HWND hwnd)
{
    char className[256]{};

    GetClassNameA(hwnd, className, sizeof(className));

    return !_stricmp(className, "ApplicationManager_DesktopShellWindow") ||

           !_stricmp(className, "WinUIDesktopWin32WindowClass");
}

static void
StampMovedOnOpeningProperty(HWND window)
{
    SetProp(window, PropertyMovedOnOpening, reinterpret_cast<HANDLE>(1));
}

static bool
RetrieveMovedOnOpeningProperty(HWND window)
{
    HANDLE handle = GetProp(window, PropertyMovedOnOpening);
    return handle != nullptr;
}

struct MonitorSignal
{
    HMONITOR Monitor{};
    ULONGLONG Tick{};
};

static std::atomic<HMONITOR> g_LastInteractionMonitor{nullptr};

static MonitorSignal g_LastTaskbarMonitor{};
static MonitorSignal g_LastMouseMonitor{};

enum class LaunchSignalSource
{
    None,
    Taskbar,
    StartMenu,
    Mouse
};

struct LaunchSignal
{
    HMONITOR Monitor{};
    ULONGLONG Tick{};
    LaunchSignalSource Source{};
};

static std::mutex g_SignalLock;
static LaunchSignal g_LastLaunchSignal{};

static ULONGLONG
NowMs()
{
    return GetTickCount64();
}

static HMONITOR
DetermineLaunchMonitor(HWND newWindow)
{
    LaunchSignal signal;

    {
        std::lock_guard<std::mutex> lock(g_SignalLock);

        signal = g_LastLaunchSignal;
    }

    printf("DEBUG launch monitor=%p tick=%llu source=%d\n", signal.Monitor, signal.Tick, (int)signal.Source);

    printf("DEBUG taskbar monitor=%p tick=%llu\n", g_LastTaskbarMonitor.Monitor, g_LastTaskbarMonitor.Tick);

    printf("DEBUG mouse monitor=%p tick=%llu\n", g_LastMouseMonitor.Monitor, g_LastMouseMonitor.Tick);

    const ULONGLONG now = NowMs();

    printf("DEBUG launch age=%llu\n", now - signal.Tick);

    printf("signal.Monitor=%p age=%llu\n", signal.Monitor, now - signal.Tick);

    //
    // Start menu / launcher surface.
    //
    if (signal.Monitor) /* &&
         (now - signal.Tick) < 1500)*/
    {
        printf("TAKING LaunchSignal branch\n");

        const char *source = "Unknown";

        switch (signal.Source)
        {
        case LaunchSignalSource::Taskbar:
            source = "Taskbar";
            break;

        case LaunchSignalSource::StartMenu:
            source = "StartMenu";
            break;

        case LaunchSignalSource::Mouse:
            source = "Mouse";
            break;
        }

        /*printf(
            "LaunchMonitor source=%s monitor=%p age=%llu\n",
            source,
            signal.Monitor,
            now - signal.Tick);*/

        printf("LaunchMonitor source=%s monitor=%p\n", source, signal.Monitor);

        return signal.Monitor;
    }

    //
    // Explicit taskbar interaction.
    //
    if (g_LastTaskbarMonitor.Monitor && (now - g_LastTaskbarMonitor.Tick) < 1500)
    {
        printf(
            "LaunchMonitor source=Taskbar monitor=%p age=%llu\n",
            g_LastTaskbarMonitor.Monitor,
            now - g_LastTaskbarMonitor.Tick);

        return g_LastTaskbarMonitor.Monitor;
    }

    //
    // Real mouse interaction.
    //
    if (g_LastMouseMonitor.Monitor && (now - g_LastMouseMonitor.Tick) < 1500)
    {
        printf(
            "LaunchMonitor source=Mouse monitor=%p age=%llu\n",
            g_LastMouseMonitor.Monitor,
            now - g_LastMouseMonitor.Tick);

        return g_LastMouseMonitor.Monitor;
    }

    HWND fg = GetForegroundWindow();

    if (fg)
    {
        if (!IsIgnoredForegroundWindow(fg))
        {
            return MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST);
        }

        printf("Ignoring foreground class during launch\n");
    }

    POINT pt{};

    if (GetCursorPos(&pt))
    {
        return MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    }

    return MonitorFromWindow(newWindow, MONITOR_DEFAULTTOPRIMARY);
}

static LRESULT CALLBACK
LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        switch (wParam)
        {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_NCLBUTTONDOWN: {
            const MSLLHOOKSTRUCT *mouse = (const MSLLHOOKSTRUCT *)lParam;

            MONITORINFOEX mi{};
            mi.cbSize = sizeof(mi);

            HMONITOR monitor = MonitorFromPoint(mouse->pt, MONITOR_DEFAULTTOPRIMARY);

            GetMonitorInfo(monitor, &mi);

            char buf[256];

            sprintf_s(buf, "CLICK MONITOR=%p (%ld,%ld)\n", monitor, mouse->pt.x, mouse->pt.y);

            printf(buf);

            HWND hwnd = WindowFromPoint(mouse->pt);

            char cls[256]{};

            GetClassNameA(hwnd, cls, sizeof(cls));

            printf("CLICK hwnd=%p class=%s monitor=%p\n", hwnd, cls, monitor);

            HWND root = GetAncestor(hwnd, GA_ROOT);

            if (GetProp(root, L"SDC_SPANNED"))
            {
                printf("PRE-RESTORE root=%p class=%s\n", root, cls);

                RestoreSpannedWindow(root);
            }

            g_LastInteractionMonitor.store(monitor, std::memory_order_relaxed);

            g_LastMouseMonitor.Monitor = monitor;
            g_LastMouseMonitor.Tick = NowMs();

            {
                std::lock_guard<std::mutex> lock(g_SignalLock);

                const bool isDesktopClick = !_stricmp(cls, "SysListView32");

                if (isDesktopClick)
                {
                    g_LastLaunchSignal = {monitor, NowMs(), LaunchSignalSource::Mouse};

                    printf("DESKTOP LAUNCH SIGNAL=%p\n", monitor);
                }
                else if (
                    g_LastLaunchSignal.Source != LaunchSignalSource::StartMenu &&
                    g_LastLaunchSignal.Source != LaunchSignalSource::Taskbar)
                {
                    printf("MOUSE SIGNAL=%p class=%s\n", monitor, cls);
                    g_LastLaunchSignal = {monitor, NowMs(), LaunchSignalSource::Mouse};
                }
            }

            break;
        }
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

static void
WindowCreated(HWND window) noexcept
{
    if (!IsProcessable(window))
    {
        return;
    }

    if (RetrieveMovedOnOpeningProperty(window))
    {
        return;
    }

    StampMovedOnOpeningProperty(window);

    HMONITOR activeMonitor = DetermineLaunchMonitor(window);

    {
        std::lock_guard<std::mutex> lock(g_SignalLock);

        printf(
            "CLEARING launch signal source=%d monitor=%p\n",
            (int)g_LastLaunchSignal.Source,
            g_LastLaunchSignal.Monitor);

        g_LastLaunchSignal = {};
    }

    char buf[256];

    sprintf_s(buf, "DetermineLaunchMonitor returned=%p\n", activeMonitor);

    printf(buf);

    if (!activeMonitor)
    {
        activeMonitor = MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    }

    m_dpiUnawareThread.submit(OnThreadExecutor::task_t{[window, activeMonitor] {
        RECT previous{};
        RECT current{};

        //
        // Wait up to 1 second for the app to settle.
        //
        int stableCount = 0;

        for (int i = 0; i < 20; i++)
        {
            RECT current{};

            if (!GetWindowRect(window, &current))
            {
                return;
            }

            if (EqualRect(&previous, &current))
            {
                stableCount++;

                if (stableCount >= 3)
                {
                    break;
                }
            }
            else
            {
                stableCount = 0;
            }

            previous = current;

            Sleep(100);
        }

        RECT r{};
        GetWindowRect(window, &r);

        char buf[512];

        sprintf_s(
            buf,
            "SETTLED hwnd=%p zoomed=%d monitor=%p rect=(%ld,%ld,%ld,%ld)\n",
            window,
            IsZoomed(window),
            MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST),
            r.left,
            r.top,
            r.right,
            r.bottom);

        printf(buf);

        OpenWindowOnActiveMonitor(window, activeMonitor);

        /*POINT cursor{};
        GetCursorPos(&cursor);

        HMONITOR cursorMonitor =
            MonitorFromPoint(
                cursor,
                MONITOR_DEFAULTTOPRIMARY);

        char buf[512];

        sprintf_s(
            buf,
            "CLICK=%p CURSOR=%p CURSORPOS=(%ld,%ld)\n",
            activeMonitor,
            cursorMonitor,
            cursor.x,
            cursor.y);

        printf(buf);

        OpenWindowOnActiveMonitor(
            window,
            cursorMonitor);*/
    }});
}

void
ShowSpanOverlay()
{
    static auto SetWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttribute_t>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

    RECT span = GetCombinedWorkAreaRect();

    constexpr int inset = 16;

    const int width = (span.right - span.left) - inset * 2;

    const int height = (span.bottom - span.top) - inset * 2;

    SetWindowPos(
        g_SpanOverlay,
        g_DragWindow,
        span.left + inset,
        span.top + inset,
        (span.right - span.left) - inset * 2,
        (span.bottom - span.top) - inset * 2,
        SWP_SHOWWINDOW);

    HRGN region = CreateRoundRectRgn(0, 0, width, height, 48, 48);

    if (SetWindowCompositionAttribute)
    {
        ACCENT_POLICY policy{};
        policy.AccentState = ACCENT_ENABLE_BLURBEHIND;

        policy.AccentFlags = 0;
        policy.GradientColor = 0;

        WINDOWCOMPOSITIONATTRIBDATA data{};
        data.Attrib = WCA_ACCENT_POLICY;

        data.pvData = &policy;

        data.cbData = sizeof(policy);

        SetWindowCompositionAttribute(g_SpanOverlay, &data);
    }

    SetWindowRgn(g_SpanOverlay, region, TRUE);

    RedrawWindow(g_SpanOverlay, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);

    DWM_SYSTEMBACKDROP_TYPE backdrop = DWMSBT_TRANSIENTWINDOW;

    DwmSetWindowAttribute(g_SpanOverlay, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
}

void
HideSpanOverlay()
{
    ShowWindow(g_SpanOverlay, SW_HIDE);
}

void
UpdateSpanPreview()
{
    if (!g_Dragging || !g_DragWindow)
    {
        return;
    }

    RECT windowRect{};

    if (!GetWindowRect(g_DragWindow, &windowRect))
    {
        return;
    }

    auto seam = GetSeamInfo();

    POINT pt{};
    GetCursorPos(&pt);

    printf("SEAM=%ld CURSOR=(%ld,%ld)\n", seam.position, pt.x, pt.y);

    LONG delta = LONG_MAX;

    if (seam.orientation == SeamOrientation::Vertical)
    {
        delta = abs(pt.x - seam.position);
    }
    else if (seam.orientation == SeamOrientation::Horizontal)
    {
        delta = abs(pt.y - seam.position);
    }

    if (seam.orientation == SeamOrientation::None)
    {
        if (g_SpanPreviewVisible)
        {
            HideSpanOverlay();
            g_SpanPreviewVisible = false;
        }

        return;
    }

    if (delta < PREVIEW_MARGIN)
    {
        if (!g_SpanPreviewVisible)
        {
            ShowSpanOverlay();

            g_SpanPreviewVisible = true;
        }
    }
    else
    {
        if (g_SpanPreviewVisible)
        {
            HideSpanOverlay();

            g_SpanPreviewVisible = false;
        }
    }
}

static HHOOK g_MouseHook = nullptr;

constexpr UINT_PTR SPAN_PREVIEW_TIMER = 0x534443;

static void CALLBACK
WinHookProc(
    HWINEVENTHOOK winEventHook,
    DWORD event,
    HWND window,
    LONG object,
    LONG child,
    DWORD eventThread,
    DWORD eventTime)
{
    UNREFERENCED_PARAMETER(winEventHook);
    UNREFERENCED_PARAMETER(child);
    UNREFERENCED_PARAMETER(eventThread);
    UNREFERENCED_PARAMETER(eventTime);

    switch (event)
    {
    case EVENT_OBJECT_NAMECHANGE:
        // The accessibility name of the desktop window changes whenever the user
        // switches virtual desktops.
        if (window == GetDesktopWindow())
        {
            VirtualDesktop::instance().UpdateVirtualDesktopId();
        }
        break;

    case EVENT_OBJECT_UNCLOAKED:
    case EVENT_OBJECT_SHOW:
    case EVENT_OBJECT_CREATE:
        if (object == OBJID_WINDOW)
        {
            WindowCreated(window);
        }
        break;
    case EVENT_SYSTEM_FOREGROUND: {
        char className[256]{};
        wchar_t title[256]{};

        GetClassNameA(window, className, sizeof(className));

        GetWindowTextW(window, title, ARRAYSIZE(title));

        HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

        if (!_stricmp(className, "Shell_TrayWnd") || !_stricmp(className, "Shell_SecondaryTrayWnd"))
        {
            g_LastTaskbarMonitor.Monitor = monitor;
            g_LastTaskbarMonitor.Tick = NowMs();

            char taskbarBuf[256];

            sprintf_s(taskbarBuf, "TASKBAR MONITOR UPDATED=%p\n", monitor);

            printf("%s", taskbarBuf);
        }

        if (!_stricmp(className, "Shell_TrayWnd") || !_stricmp(className, "Shell_SecondaryTrayWnd"))
        {
            std::lock_guard<std::mutex> lock(g_SignalLock);

            if (g_LastLaunchSignal.Source != LaunchSignalSource::Taskbar)
            {
                g_LastLaunchSignal = {monitor, NowMs(), LaunchSignalSource::Taskbar};

                printf("TASKBAR LAUNCH SIGNAL=%p\n", monitor);
            }
        }

        if (!_stricmp(className, "Windows.UI.Core.CoreWindow"))
        {
            if (!_wcsicmp(title, L"Start"))
            {
                {
                    std::lock_guard<std::mutex> lock(g_SignalLock);

                    g_LastLaunchSignal = {monitor, NowMs(), LaunchSignalSource::StartMenu};
                }

                printf("START MONITOR UPDATED=%p\n", monitor);
            }
        }

        if (!_stricmp(className, "Windows.UI.Core.CoreWindow"))
        {
            char buf[1024];

            sprintf_s(buf, "COREWINDOW hwnd=%p monitor=%p title=%ws\n", window, monitor, title);

            printf("%s", buf);
        }

        MONITORINFOEX mi{};
        mi.cbSize = sizeof(mi);

        GetMonitorInfo(monitor, &mi);

        char buf[2048];

        sprintf_s(
            buf,
            "FOREGROUND hwnd=%p class=%s "
            "monitor=%p "
            "monitorRect=(%ld,%ld,%ld,%ld)\n",
            window,
            className,
            monitor,
            mi.rcMonitor.left,
            mi.rcMonitor.top,
            mi.rcMonitor.right,
            mi.rcMonitor.bottom);

        printf("%s", buf);

        break;
    }
    case EVENT_SYSTEM_MOVESIZESTART: {
        printf("MOVESIZESTART hwnd=%p class=", window);

        char cls[256]{};
        GetClassNameA(window, cls, sizeof(cls));

        printf("%s\n", cls);

        if (GetProp(window, L"SDC_SPANNED"))
        {
            RestoreSpannedWindow(window);
        }

        HideSpanOverlay();
        g_SpanPreviewVisible = false;

        g_Dragging = true;
        g_DragWindow = window;

        SetTimer(nullptr, SPAN_PREVIEW_TIMER, 16, [](HWND, UINT, UINT_PTR, DWORD) { UpdateSpanPreview(); });

        break;
    }
    case EVENT_SYSTEM_MOVESIZEEND: {
        if (g_Dragging)
        {
            KillTimer(nullptr, SPAN_PREVIEW_TIMER);

            auto seam = GetSeamInfo();

            HideSpanOverlay();
            g_SpanPreviewVisible = false;

            if (!g_DragWindow)
            {
                break;
            }

            RECT windowRect{};
            GetWindowRect(g_DragWindow, &windowRect);

            LONG delta = LONG_MAX;

            POINT pt{};
            GetCursorPos(&pt);

            printf("SEAM=%ld CURSOR=(%ld,%ld)\n", seam.position, pt.x, pt.y);

            if (seam.orientation == SeamOrientation::Vertical)
            {
                delta = abs(pt.x - seam.position);
            }
            else if (seam.orientation == SeamOrientation::Horizontal)
            {
                delta = abs(pt.y - seam.position);
            }

            if (seam.orientation == SeamOrientation::None)
            {
                g_Dragging = false;
                g_DragWindow = nullptr;
                break;
            }

            if (delta < COMMIT_MARGIN && !GetProp(window, L"SDC_SPANNED"))
            {
                SpanOrCenterWindow(window);
            }

            g_Dragging = false;
            g_DragWindow = nullptr;
        }
        break;
    }
    case EVENT_OBJECT_DESTROY: {
        g_SpanStates.erase(window);

        RemoveProp(window, L"SDC_SPANNED");

        break;
    }
    }
}

static LRESULT CALLBACK
OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;
        /*case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc{};
            GetClientRect(hwnd, &rc);

            FillRect(
                hdc,
                &rc,
                (HBRUSH)GetStockObject(BLACK_BRUSH));

            HBRUSH brush =
                CreateSolidBrush(
                    RGB(240,240,240));

            HPEN pen =
                (HPEN)GetStockObject(NULL_PEN);

            auto oldPen =
                SelectObject(hdc, pen);

            auto oldBrush =
                SelectObject(hdc, brush);

            RoundRect(
                hdc,
                0,
                0,
                rc.right,
                rc.bottom,
                16,
                16);

            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);

            DeleteObject(brush);

            EndPaint(hwnd, &ps);
            return 0;
        }*/
        /*case WM_PAINT:
         {
             PAINTSTRUCT ps{};
             BeginPaint(hwnd, &ps);
             EndPaint(hwnd, &ps);
             return 0;
         }*/
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc{};
        GetClientRect(hwnd, &rc);

        HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));

        BLENDFUNCTION blend{};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 12; // experiment

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

    case WM_NCHITTEST:
        return HTTRANSPARENT;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

VOID
ActiveMonitorWindowHandlerMain()
{
    CoInitialize(NULL);

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    POINT pt = {0, 0};

    g_LastInteractionMonitor.store(MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY), std::memory_order_relaxed);

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"SpanOverlay";

    RegisterClassEx(&wc);

    g_SpanOverlay = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        L"SpanOverlay",
        L"",
        WS_POPUP,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_ROUND;

    DwmSetWindowAttribute(g_SpanOverlay, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));

    MARGINS margins = {-1};

    DwmExtendFrameIntoClientArea(g_SpanOverlay, &margins);

    BOOL enable = TRUE;

    DwmSetWindowAttribute(g_SpanOverlay, DWMWA_NCRENDERING_ENABLED, &enable, sizeof(enable));

    g_MouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);

    std::array<DWORD, 8> events_to_subscribe = {
        EVENT_SYSTEM_FOREGROUND,
        EVENT_OBJECT_NAMECHANGE,
        EVENT_OBJECT_UNCLOAKED,
        EVENT_OBJECT_SHOW,
        EVENT_OBJECT_CREATE,
        EVENT_SYSTEM_MOVESIZESTART,
        EVENT_SYSTEM_MOVESIZEEND,
        EVENT_OBJECT_DESTROY};

    for (const auto event : events_to_subscribe)
    {
        auto hook =
            SetWinEventHook(event, event, nullptr, WinHookProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
        if (hook)
        {
            m_staticWinEventHooks.emplace_back(hook);
        }
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_QUIT)
        {
            break;
        }

        if (msg.message == WM_DISPLAYCHANGE || (msg.message == WM_SETTINGCHANGE && msg.wParam == SPI_SETWORKAREA))
        {
            UpdateMonitorWorkAreas();
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    m_staticWinEventHooks.erase(
        std::remove_if(
            begin(m_staticWinEventHooks),
            end(m_staticWinEventHooks),
            [](const HWINEVENTHOOK hook) { return UnhookWinEvent(hook); }),
        end(m_staticWinEventHooks));

    if (g_MouseHook)
    {
        UnhookWindowsHookEx(g_MouseHook);
        g_MouseHook = NULL;
    }

    CoUninitialize();
}