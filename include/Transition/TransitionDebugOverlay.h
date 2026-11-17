#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace Transition {

struct DebugRegion
{
    RECT Rect{};

    std::wstring Label;

    COLORREF Color = RGB(255, 255, 255);
};

class TransitionDebugOverlay
{
public:
    TransitionDebugOverlay() = default;
    ~TransitionDebugOverlay() = default;

    HRESULT Initialize(HWND ParentWindow);

    void Shutdown();

    void Show();

    void Hide();

    void Clear();

    void AddRegion(const DebugRegion &Region);

    void SetStatusText(const std::wstring &Text);

    void SetEnabled(bool Enabled) noexcept { m_Enabled = Enabled; }

    bool IsEnabled() const noexcept { return m_Enabled; }

    HWND GetWindow() const noexcept { return m_Window; }

private:
    static LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

    void OnPaint();

private:
    HWND m_Window = nullptr;

    HWND m_ParentWindow = nullptr;

    bool m_Enabled = false;

    std::wstring m_StatusText;

    std::vector<DebugRegion> m_Regions;
};

} // namespace Transition