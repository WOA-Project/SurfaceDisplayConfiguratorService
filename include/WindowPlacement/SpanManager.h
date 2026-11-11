#pragma once

#include <Windows.h>

#include <unordered_map>

#include "../Models/SpanState.h"

class SpanManager
{
public:
    void SpanWindow(HWND window, const RECT &spanRect);

    void RestoreWindow(HWND window);

    bool IsSpanned(HWND window) const;

    void RemoveWindow(HWND window);

private:
    void SetSpanAppearance(HWND window, bool enabled);

private:
    std::unordered_map<HWND, SpanState> m_spanStates;
};