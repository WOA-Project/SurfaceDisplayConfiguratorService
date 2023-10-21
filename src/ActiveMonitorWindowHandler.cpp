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
#include "VirtualDesktop.h"
#include <on_thread_executor.h>
#include <tchar.h>

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

static BOOL CALLBACK
saveDisplayToVector(HMONITOR monitor, HDC /*hdc*/, LPRECT /*rect*/, LPARAM data)
{
    reinterpret_cast<std::vector<HMONITOR> *>(data)->emplace_back(monitor);
    return true;
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

    if (S_OK !=
        GetDpiForMonitor(monitors[0], MDT_EFFECTIVE_DPI, &firstMonitorDpiX, &firstMonitorDpiY))
    {
        return false;
    }

    for (int i = 1; i < monitors.size(); i++)
    {
        UINT iteratedMonitorDpiX;
        UINT iteratedMonitorDpiY;

        if (S_OK != GetDpiForMonitor(
                        monitors[i], MDT_EFFECTIVE_DPI, &iteratedMonitorDpiX, &iteratedMonitorDpiY) ||
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

static void
SizeWindowToRect(HWND window, RECT rect) noexcept
{
    WINDOWPLACEMENT placement{};
    GetWindowPlacement(window, &placement);

    // Wait if SW_SHOWMINIMIZED would be removed from window (Issue #1685)
    for (int i = 0; i < 5 && (placement.showCmd == SW_SHOWMINIMIZED); ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        GetWindowPlacement(window, &placement);
    }

    if (!IsWindowVisible(window))
    {
        placement.showCmd = SW_HIDE;
    }
    else
    {
        // Do not restore minimized windows. We change their placement though so they restore to the correct zone.
        if ((placement.showCmd != SW_SHOWMINIMIZED) && (placement.showCmd != SW_MINIMIZE))
        {
            placement.showCmd = SW_RESTORE;
        }

        // Remove maximized show command to make sure window is moved to the correct zone.
        if (placement.showCmd == SW_SHOWMAXIMIZED)
        {
            placement.showCmd = SW_RESTORE;
            placement.flags &= ~WPF_RESTORETOMAXIMIZED;
        }
    }

    ScreenToWorkAreaCoords(window, rect);

    placement.rcNormalPosition = rect;
    placement.flags |= WPF_ASYNCWINDOWPLACEMENT;

    SetWindowPlacement(window, &placement);

    // Do it again, allowing Windows to resize the window and set correct scaling
    // This fixes Issue #365
    SetWindowPlacement(window, &placement);
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
FitOnScreen(const RECT &windowRect, const RECT &originMonitorRect, const RECT &destMonitorRect)
{
    // New window position on active monitor. If window fits the screen, this will be final position.
    int left = destMonitorRect.left + (windowRect.left - originMonitorRect.left);
    int top = destMonitorRect.top + (windowRect.top - originMonitorRect.top);
    int W = RectWidth(windowRect);
    int H = RectHeight(windowRect);

    if ((left < destMonitorRect.left) || (left + W > destMonitorRect.right))
    {
        // Set left window border to left border of screen (add padding). Resize window width if needed.
        left = destMonitorRect.left + CUSTOM_POSITIONING_LEFT_TOP_PADDING;
        W = min(W, RectWidth(destMonitorRect) - CUSTOM_POSITIONING_LEFT_TOP_PADDING);
    }
    if ((top < destMonitorRect.top) || (top + H > destMonitorRect.bottom))
    {
        // Set top window border to top border of screen (add padding). Resize window height if needed.
        top = destMonitorRect.top + CUSTOM_POSITIONING_LEFT_TOP_PADDING;
        H = min(H, RectHeight(destMonitorRect) - CUSTOM_POSITIONING_LEFT_TOP_PADDING);
    }

    return {.left = left, .top = top, .right = left + W, .bottom = top + H};
}

static void
OpenWindowOnActiveMonitor(HWND window, HMONITOR monitor) noexcept
{
    // By default Windows opens new window on primary monitor.
    // Try to preserve window width and height, adjust top-left corner if needed.
    HMONITOR origin = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
    if (origin == monitor)
    {
        // Certain applications by design open in last known position, regardless of FancyZones.
        // If that position is on currently active monitor, skip custom positioning.
        return;
    }

    WINDOWPLACEMENT placement{};
    if (GetWindowPlacement(window, &placement))
    {
        MONITORINFOEX originMi;
        originMi.cbSize = sizeof(originMi);
        if (GetMonitorInfo(origin, &originMi))
        {
            MONITORINFOEX destMi;
            destMi.cbSize = sizeof(destMi);
            if (GetMonitorInfo(monitor, &destMi))
            {
                RECT newPosition = FitOnScreen(placement.rcNormalPosition, originMi.rcWork, destMi.rcWork);
                SizeWindowToRect(window, newPosition);
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
    auto process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, TRUE, pid);
    std::wstring name;
    if (process != INVALID_HANDLE_VALUE)
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
    const bool isSplashScreen = IsSplashScreen(window);
    if (isSplashScreen)
    {
        return false;
    }

    const bool windowMinimized = IsIconic(window);
    if (windowMinimized)
    {
        return false;
    }

    const bool standard = IsStandardWindow(window);
    if (!standard)
    {
        return false;
    }

    // popup could be the window we don't want to snap: start menu, notification popup, tray window, etc.
    // also, popup could be the windows we want to snap disregarding the "allowSnapPopupWindows" setting, e.g. Telegram
    bool isPopup = IsPopupWindow(window);
    bool hasThickFrame = HasThickFrame(window);
    if (isPopup && !hasThickFrame)
    {
        return false;
    }

    // allow child windows
    auto hasOwner = HasVisibleOwner(window);
    if (hasOwner)
    {
        return false;
    }

    if (IsExcluded(window))
    {
        return false;
    }

    // Switch between virtual desktops results with posting same windows messages that also indicate
    // creation of new window. We need to check if window being processed is on currently active desktop.
    if (!VirtualDesktop::instance().IsWindowOnCurrentDesktop(window))
    {
        return false;
    }

    return true;
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

static void
WindowCreated(HWND window) noexcept
{
    if (!IsProcessable(window))
    {
        return;
    }

    HMONITOR primary = MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY);
    HMONITOR active = primary;

    POINT cursorPosition{};
    if (GetCursorPos(&cursorPosition))
    {
        active = MonitorFromPoint(cursorPosition, MONITOR_DEFAULTTOPRIMARY);
    }

    // window is recreated after switching virtual desktop
    // avoid moving already opened windows after switching vd
    bool isMoved = RetrieveMovedOnOpeningProperty(window);
    if (!isMoved)
    {
        StampMovedOnOpeningProperty(window);
        m_dpiUnawareThread.submit(OnThreadExecutor::task_t{[&] { OpenWindowOnActiveMonitor(window, active); }}).wait();
    }
}

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

    POINT ptScreen;
    GetPhysicalCursorPos(&ptScreen);

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
            const auto wparam = reinterpret_cast<WPARAM>(window);
            auto hwnd = reinterpret_cast<HWND>(wparam);
            WindowCreated(hwnd);
        }
        break;
    }
}

VOID
ActiveMonitorWindowHandlerMain()
{
    CoInitialize(NULL);

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    std::array<DWORD, 6> events_to_subscribe = {
        EVENT_OBJECT_NAMECHANGE,
        EVENT_OBJECT_UNCLOAKED,
        EVENT_OBJECT_SHOW,
        EVENT_OBJECT_CREATE};
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
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    m_staticWinEventHooks.erase(
        std::remove_if(
            begin(m_staticWinEventHooks),
            end(m_staticWinEventHooks),
            [](const HWINEVENTHOOK hook) { return UnhookWinEvent(hook); }),
        end(m_staticWinEventHooks));

    CoUninitialize();
}