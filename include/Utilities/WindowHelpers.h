#pragma once

#include <Windows.h>

#include <string>

namespace WindowHelpers {
bool
Exists(HWND hwnd);

bool
IsMaximized(HWND hwnd);

bool
IsMinimized(HWND hwnd);

bool
IsVisible(HWND hwnd);

bool
IsTopLevelWindow(HWND hwnd);

RECT
GetWindowRectSafe(HWND hwnd);

WINDOWPLACEMENT
GetPlacement(HWND hwnd);

HWND
GetRootWindow(HWND hwnd);

HMONITOR
GetMonitor(HWND hwnd);

bool
HasStyle(HWND hwnd, LONG_PTR style);

bool
HasExStyle(HWND hwnd, LONG_PTR style);

DWORD
GetProcessId(HWND hwnd);

std::wstring
GetClassNameString(HWND hwnd);

std::wstring
GetWindowTitle(HWND hwnd);
} // namespace WindowHelpers