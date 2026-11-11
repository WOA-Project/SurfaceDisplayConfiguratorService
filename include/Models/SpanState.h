#pragma once

#include <Windows.h>

struct SpanState
{
    SpanState() { Placement.length = sizeof(WINDOWPLACEMENT); }

    WINDOWPLACEMENT Placement{};
};