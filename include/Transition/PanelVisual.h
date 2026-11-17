#pragma once

#include <windows.h>

#include <dcomp.h>
#include <wrl/client.h>

namespace Transition {

struct PanelVisual
{
    RECT DesktopRect{};

    UINT Width = 0;

    UINT Height = 0;

    Microsoft::WRL::ComPtr<IDCompositionVisual> Visual;

    Microsoft::WRL::ComPtr<IDCompositionSurface> Surface;

    Microsoft::WRL::ComPtr<IDCompositionTranslateTransform> Translate;

    Microsoft::WRL::ComPtr<IDCompositionScaleTransform> Scale;

    Microsoft::WRL::ComPtr<IDCompositionRotateTransform> Rotate;

#if !TRANSITION_DISABLE_TRANSFORMGROUPS

    Microsoft::WRL::ComPtr<IDCompositionTransform> TransformGroup;

#endif
};

} // namespace Transition