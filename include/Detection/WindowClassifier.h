#pragma once

#include <Windows.h>

#include <string>

class WindowClassifier
{
public:
    bool IsProcessable(HWND window);

private:
    static bool IsSplashScreen(HWND window);

    static bool IsStandardWindow(HWND window);

    static bool IsPopupWindow(HWND window);

    static bool HasThickFrame(HWND window);

    static bool HasVisibleOwner(HWND window);

    static bool IsExcluded(HWND window);

    static bool IsExcludedByDefault(HWND hwnd, const std::wstring &processPath);

    static std::wstring GetProcessPath(HWND window);

    static std::wstring GetProcessPath(DWORD pid);
};