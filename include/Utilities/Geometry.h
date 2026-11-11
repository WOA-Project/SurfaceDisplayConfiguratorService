#pragma once

#include <Windows.h>

namespace Geometry {
int
RectWidth(const RECT &rect);

int
RectHeight(const RECT &rect);

RECT
GetVirtualDesktopRect();

bool
IsEmpty(const RECT &rect);

POINT
GetCenter(const RECT &rect);

RECT
CenterRectInRect(const RECT &inner, const RECT &outer);

RECT
OffsetRectCopy(const RECT &rect, int dx, int dy);

RECT
InflateRectCopy(const RECT &rect, int dx, int dy);

bool
ContainsPoint(const RECT &rect, POINT point);

RECT
ConstrainToBounds(const RECT &rect, const RECT &bounds);

RECT
ClampSizeToBounds(const RECT &rect, const RECT &bounds);
} // namespace Geometry