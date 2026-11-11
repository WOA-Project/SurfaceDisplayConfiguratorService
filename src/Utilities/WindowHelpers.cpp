#include "pch.h"
#include "Utilities/WindowHelpers.h"

namespace WindowHelpers {
bool
Exists(HWND hwnd)
{
    return hwnd && ::IsWindow(hwnd);
}

bool
IsMaximized(HWND hwnd)
{
    return Exists(hwnd) && ::IsZoomed(hwnd);
}

bool
IsMinimized(HWND hwnd)
{
    return Exists(hwnd) && ::IsIconic(hwnd);
}

bool
IsVisible(HWND hwnd)
{
    return Exists(hwnd) && ::IsWindowVisible(hwnd);
}

bool
IsTopLevelWindow(HWND hwnd)
{
    return Exists(hwnd) && ::GetAncestor(hwnd, GA_ROOT) == hwnd;
}

RECT
GetWindowRectSafe(HWND hwnd)
{
    RECT rect{};

    if (!Exists(hwnd))
    {
        return rect;
    }

    ::GetWindowRect(hwnd, &rect);

    return rect;
}

WINDOWPLACEMENT
GetPlacement(HWND hwnd)
{
    WINDOWPLACEMENT placement{};
    placement.length = sizeof(WINDOWPLACEMENT);

    if (!Exists(hwnd))
    {
        return placement;
    }

    ::GetWindowPlacement(hwnd, &placement);

    return placement;
}

HWND
GetRootWindow(HWND hwnd)
{
    if (!Exists(hwnd))
    {
        return nullptr;
    }

    HWND root = ::GetAncestor(hwnd, GA_ROOT);

    return root ? root : hwnd;
}

HMONITOR
GetMonitor(HWND hwnd)
{
    if (!Exists(hwnd))
    {
        return nullptr;
    }

    return ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
}

bool
HasStyle(HWND hwnd, LONG_PTR style)
{
    if (!Exists(hwnd))
    {
        return false;
    }

    const LONG_PTR current = ::GetWindowLongPtr(hwnd, GWL_STYLE);

    return (current & style) == style;
}

bool
HasExStyle(HWND hwnd, LONG_PTR style)
{
    if (!Exists(hwnd))
    {
        return false;
    }

    const LONG_PTR current = ::GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    return (current & style) == style;
}

DWORD
GetProcessId(HWND hwnd)
{
    if (!Exists(hwnd))
    {
        return 0;
    }

    DWORD pid{};

    ::GetWindowThreadProcessId(hwnd, &pid);

    return pid;
}

std::wstring
GetClassNameString(HWND hwnd)
{
    if (!Exists(hwnd))
    {
        return {};
    }

    wchar_t className[256]{};

    ::GetClassNameW(hwnd, className, ARRAYSIZE(className));

    return className;
}

std::wstring
GetWindowTitle(HWND hwnd)
{
    if (!Exists(hwnd))
    {
        return {};
    }

    wchar_t title[512]{};

    ::GetWindowTextW(hwnd, title, ARRAYSIZE(title));

    return title;
}
} // namespace WindowHelpers