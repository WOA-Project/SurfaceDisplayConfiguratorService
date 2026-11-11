#pragma once

#include <Windows.h>

#include <mutex>

#include "../Models/LaunchSignal.h"
#include "../Models/MonitorSignal.h"

class LaunchMonitorResolver
{
public:
    void RecordMouseClick(const POINT &point, HWND targetWindow);

    void RecordForegroundWindow(HWND window);

    HMONITOR DetermineLaunchMonitor(HWND newWindow);

    void ClearLaunchSignal();

private:
    static ULONGLONG NowMs();

private:
    MonitorSignal m_lastTaskbarMonitor{};

    MonitorSignal m_lastMouseMonitor{};

    LaunchSignal m_lastLaunchSignal{};

    std::mutex m_lock;
};