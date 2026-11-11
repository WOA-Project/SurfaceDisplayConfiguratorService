#pragma once

#include <Windows.h>

enum class SeamOrientation
{
    None,
    Vertical,
    Horizontal
};

struct SeamInfo
{
    SeamOrientation Orientation = SeamOrientation::None;

    LONG Position = 0;

    RECT ActivationZone{};
};