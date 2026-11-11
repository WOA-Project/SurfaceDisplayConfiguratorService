#include "pch.h"
#include "WindowPlacement/SpanManager.h"

#include <dwmapi.h>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE

enum DWM_WINDOW_CORNER_PREFERENCE
{
    DWMWCP_DEFAULT = 0,
    DWMWCP_DONOTROUND = 1,
    DWMWCP_ROUND = 2,
    DWMWCP_ROUNDSMALL = 3
};

#    define DWMWA_WINDOW_CORNER_PREFERENCE 33

#endif

namespace {
constexpr wchar_t SpannedProperty[] = L"SDC_SPANNED";
}

void
SpanManager::SetSpanAppearance(HWND window, bool enabled)
{
    if (!IsWindow(window))
    {
        return;
    }

    DWM_WINDOW_CORNER_PREFERENCE preference = enabled ? DWMWCP_DONOTROUND : DWMWCP_DEFAULT;

    DwmSetWindowAttribute(window, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
}

bool
SpanManager::IsSpanned(HWND window) const
{
    return m_spanStates.find(window) != m_spanStates.end();
}

void
SpanManager::RemoveWindow(HWND window)
{
    m_spanStates.erase(window);

    RemoveProp(window, SpannedProperty);
}

void
SpanManager::RestoreWindow(HWND window)
{
    auto it = m_spanStates.find(window);

    if (it == m_spanStates.end())
    {
        return;
    }

    if (!IsWindow(window))
    {
        m_spanStates.erase(it);
        return;
    }

    SetSpanAppearance(window, false);

    DwmFlush();

    WINDOWPLACEMENT placement = it->second.Placement;

    placement.length = sizeof(placement);

    RECT currentRect{};

    if (!GetWindowRect(window, &currentRect))
    {
        m_spanStates.erase(it);
        return;
    }

    const int currentWidth = currentRect.right - currentRect.left;

    POINT cursor{};

    if (!GetCursorPos(&cursor))
    {
        cursor.x = currentRect.left + currentWidth / 2;

        cursor.y = currentRect.top + 32;
    }

    RECT restoreRect = placement.rcNormalPosition;

    const int restoreWidth = restoreRect.right - restoreRect.left;

    const int restoreHeight = restoreRect.bottom - restoreRect.top;

    double grabRatio = 0.5;

    if (currentWidth > 0)
    {
        grabRatio = static_cast<double>(cursor.x - currentRect.left) / static_cast<double>(currentWidth);

        grabRatio = std::clamp(grabRatio, 0.0, 1.0);
    }

    RECT target{};

    target.left = cursor.x - static_cast<LONG>(grabRatio * restoreWidth);

    const int cursorOffsetY = cursor.y - currentRect.top;

    const int titleBarOffsetY = std::clamp(cursorOffsetY, 16, 40);

    target.top = cursor.y - titleBarOffsetY;

    target.right = target.left + restoreWidth;

    target.bottom = target.top + restoreHeight;

    placement.rcNormalPosition = target;

    placement.showCmd = SW_RESTORE;

    SetWindowPlacement(window, &placement);

    ShowWindow(window, SW_RESTORE);

    SetWindowPos(
        window,
        nullptr,

        target.left,
        target.top,

        restoreWidth,
        restoreHeight,

        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);

    RemoveProp(window, SpannedProperty);

    m_spanStates.erase(it);
}

void
SpanManager::SpanWindow(HWND window, const RECT &spanRect)
{
    if (!IsWindow(window))
    {
        return;
    }

    if (IsSpanned(window))
    {
        return;
    }

    RECT previousRect{};

    if (!GetWindowRect(window, &previousRect))
    {
        return;
    }

    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);

    if (!GetWindowPlacement(window, &placement))
    {
        return;
    }

    const bool wasMaximized = IsZoomed(window);

    if (wasMaximized)
    {
        ShowWindow(window, SW_RESTORE);

        placement.length = sizeof(placement);

        if (!GetWindowPlacement(window, &placement))
        {
            return;
        }
    }

    SpanState state;
    state.Placement = placement;

    m_spanStates[window] = state;

    SetSpanAppearance(window, true);

    const int borderX = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

    const int borderY = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

    const int width = (spanRect.right - spanRect.left) + borderX * 2;

    const int height = (spanRect.bottom - spanRect.top) + borderY + 2;

    SetWindowPos(
        window,
        nullptr,

        spanRect.left - borderX,
        spanRect.top - 2,

        width,
        height,

        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);

    DwmFlush();

    RECT actualRect{};

    if (!GetWindowRect(window, &actualRect))
    {
        return;
    }

    const int originalWidth = previousRect.right - previousRect.left;

    const int originalHeight = previousRect.bottom - previousRect.top;

    const int actualWidth = actualRect.right - actualRect.left;

    const int actualHeight = actualRect.bottom - actualRect.top;

    const bool resizeIgnored = actualWidth < (originalWidth * 3 / 2) && actualHeight < (originalHeight * 3 / 2);

    if (resizeIgnored)
    {
        const int spanWidth = spanRect.right - spanRect.left;

        const int spanHeight = spanRect.bottom - spanRect.top;

        const int centerX = spanRect.left + (spanWidth - actualWidth) / 2;

        const int centerY = spanRect.top + (spanHeight - actualHeight) / 2;

        SetWindowPos(
            window,
            nullptr,

            centerX,
            centerY,

            0,
            0,

            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    SetProp(window, SpannedProperty, reinterpret_cast<HANDLE>(1));
}