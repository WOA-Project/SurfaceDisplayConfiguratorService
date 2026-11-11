#include "pch.h"
#include "Detection/LaunchMonitorResolver.h"
#include "Monitor/MonitorHelpers.h"

namespace {
constexpr ULONGLONG SignalTimeoutMs = 1500;

bool
IsIgnoredForegroundWindow(HWND hwnd)
{
    char className[256]{};

    GetClassNameA(hwnd, className, sizeof(className));

    return !_stricmp(className, "ApplicationManager_DesktopShellWindow") ||
           !_stricmp(className, "WinUIDesktopWin32WindowClass");
}
} // namespace

ULONGLONG
LaunchMonitorResolver::NowMs()
{
    return GetTickCount64();
}

void
LaunchMonitorResolver::ClearLaunchSignal()
{
    std::lock_guard<std::mutex> lock(m_lock);

    m_lastLaunchSignal = {};
}

void
LaunchMonitorResolver::RecordMouseClick(const POINT &point, HWND targetWindow)
{
    const ULONGLONG now = NowMs();

    HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);

    {
        std::lock_guard<std::mutex> lock(m_lock);

        m_lastMouseMonitor = {monitor, now};
    }

    char className[256]{};

    GetClassNameA(targetWindow, className, sizeof(className));

    const bool desktopClick = !_stricmp(className, "SysListView32");

    std::lock_guard<std::mutex> lock(m_lock);

    if (desktopClick)
    {
        m_lastLaunchSignal = {monitor, now, LaunchSignalSource::Mouse};

        return;
    }

    if (m_lastLaunchSignal.Source != LaunchSignalSource::Taskbar &&
        m_lastLaunchSignal.Source != LaunchSignalSource::StartMenu)
    {
        m_lastLaunchSignal = {monitor, now, LaunchSignalSource::Mouse};
    }
}

void
LaunchMonitorResolver::RecordForegroundWindow(HWND window)
{
    if (!window)
    {
        return;
    }

    const ULONGLONG now = NowMs();

    char className[256]{};

    GetClassNameA(window, className, sizeof(className));

    HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

    //
    // Taskbar.
    //
    if (!_stricmp(className, "Shell_TrayWnd") || !_stricmp(className, "Shell_SecondaryTrayWnd"))
    {
        std::lock_guard<std::mutex> lock(m_lock);

        m_lastTaskbarMonitor = {monitor, now};

        m_lastLaunchSignal = {monitor, now, LaunchSignalSource::Taskbar};

        return;
    }

    //
    // Start menu.
    //
    if (!_stricmp(className, "Windows.UI.Core.CoreWindow"))
    {
        wchar_t title[256]{};

        GetWindowTextW(window, title, ARRAYSIZE(title));

        if (!_wcsicmp(title, L"Start"))
        {
            std::lock_guard<std::mutex> lock(m_lock);

            m_lastLaunchSignal = {monitor, now, LaunchSignalSource::StartMenu};
        }
    }
}

HMONITOR
LaunchMonitorResolver::DetermineLaunchMonitor(HWND newWindow)
{
    const ULONGLONG now = NowMs();

    LaunchSignal signal{};
    MonitorSignal taskbarSignal{};
    MonitorSignal mouseSignal{};

    {
        std::lock_guard<std::mutex> lock(m_lock);

        signal = m_lastLaunchSignal;

        taskbarSignal = m_lastTaskbarMonitor;

        mouseSignal = m_lastMouseMonitor;
    }

    //
    // Explicit launch signal.
    //
    if (signal.Monitor && MonitorHelpers::IsMonitorValid(signal.Monitor))
    {
        return signal.Monitor;
    }

    //
    // Recent taskbar interaction.
    //
    if (taskbarSignal.Monitor && MonitorHelpers::IsMonitorValid(taskbarSignal.Monitor) &&
        (now - taskbarSignal.Tick) < SignalTimeoutMs)
    {
        return taskbarSignal.Monitor;
    }

    //
    // Recent mouse interaction.
    //
    if (mouseSignal.Monitor && MonitorHelpers::IsMonitorValid(mouseSignal.Monitor) &&
        (now - mouseSignal.Tick) < SignalTimeoutMs)
    {
        return mouseSignal.Monitor;
    }

    //
    // Current foreground window.
    //
    HWND foreground = GetForegroundWindow();

    if (foreground && !IsIgnoredForegroundWindow(foreground))
    {
        HMONITOR monitor = MonitorFromWindow(foreground, MONITOR_DEFAULTTONEAREST);

        if (monitor)
        {
            return monitor;
        }
    }

    //
    // Cursor monitor.
    //
    POINT cursor{};

    if (GetCursorPos(&cursor))
    {
        return MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
    }

    //
    // Window monitor fallback.
    //
    return MonitorFromWindow(newWindow, MONITOR_DEFAULTTOPRIMARY);
}