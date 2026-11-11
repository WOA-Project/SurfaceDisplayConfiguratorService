#pragma once

#include <Windows.h>

enum class LaunchSignalSource
{
    None,
    Taskbar,
    StartMenu,
    Mouse
};

struct LaunchSignal
{
    HMONITOR
    Monitor{};

    ULONGLONG
    Tick{};

    LaunchSignalSource Source = LaunchSignalSource::None;
};