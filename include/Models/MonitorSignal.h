#pragma once

#include <Windows.h>

struct MonitorSignal
{
    HMONITOR
    Monitor{};

    ULONGLONG
    Tick{};
};