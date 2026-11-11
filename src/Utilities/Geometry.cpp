#include "pch.h"
#include "Utilities/Geometry.h"
#include "Monitor/MonitorHelpers.h"

namespace Geometry {
int
RectWidth(const RECT &rect)
{
    return rect.right - rect.left;
}

int
RectHeight(const RECT &rect)
{
    return rect.bottom - rect.top;
}

RECT
GetVirtualDesktopRect()
{
    return MonitorHelpers::GetVirtualDesktopRect();
}

bool
IsEmpty(const RECT &rect)
{
    return RectWidth(rect) <= 0 || RectHeight(rect) <= 0;
}

POINT
GetCenter(const RECT &rect)
{
    POINT center{};

    center.x = rect.left + RectWidth(rect) / 2;

    center.y = rect.top + RectHeight(rect) / 2;

    return center;
}

RECT
CenterRectInRect(const RECT &inner, const RECT &outer)
{
    RECT result{};

    const LONG width = RectWidth(inner);

    const LONG height = RectHeight(inner);

    result.left = outer.left + (RectWidth(outer) - width) / 2;

    result.top = outer.top + (RectHeight(outer) - height) / 2;

    result.right = result.left + width;

    result.bottom = result.top + height;

    return result;
}

RECT
OffsetRectCopy(const RECT &rect, int dx, int dy)
{
    RECT result = rect;

    ::OffsetRect(&result, dx, dy);

    return result;
}

RECT
InflateRectCopy(const RECT &rect, int dx, int dy)
{
    RECT result = rect;

    ::InflateRect(&result, dx, dy);

    return result;
}

bool
ContainsPoint(const RECT &rect, POINT point)
{
    //
    // Win32 RECT semantics:
    // left/top inclusive,
    // right/bottom exclusive.
    //
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

RECT
ClampSizeToBounds(const RECT &rect, const RECT &bounds)
{
    RECT result = rect;

    const LONG maxWidth = RectWidth(bounds);

    const LONG maxHeight = RectHeight(bounds);

    if (RectWidth(result) > maxWidth)
    {
        result.right = result.left + maxWidth;
    }

    if (RectHeight(result) > maxHeight)
    {
        result.bottom = result.top + maxHeight;
    }

    return result;
}

RECT
ConstrainToBounds(const RECT &rect, const RECT &bounds)
{
    RECT result = ClampSizeToBounds(rect, bounds);

    const LONG width = RectWidth(result);

    const LONG height = RectHeight(result);

    if (result.left < bounds.left)
    {
        result.left = bounds.left;

        result.right = result.left + width;
    }

    if (result.right > bounds.right)
    {
        result.right = bounds.right;

        result.left = result.right - width;
    }

    if (result.top < bounds.top)
    {
        result.top = bounds.top;

        result.bottom = result.top + height;
    }

    if (result.bottom > bounds.bottom)
    {
        result.bottom = bounds.bottom;

        result.top = result.bottom - height;
    }

    return result;
}
} // namespace Geometry