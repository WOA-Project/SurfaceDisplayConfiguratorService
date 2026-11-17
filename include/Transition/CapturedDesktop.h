#pragma once

#include <windows.h>

#include <d3d11.h>
#include <wrl/client.h>

#include <vector>

namespace Transition {

struct CapturedPanel
{
    UINT OutputIndex = 0;

    RECT DesktopRect{};

    Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
};

struct CapturedDesktop
{
    std::vector<CapturedPanel> Panels;

    void Clear() { Panels.clear(); }

    bool Empty() const noexcept { return Panels.empty(); }

    size_t Count() const noexcept { return Panels.size(); }
};

} // namespace Transition