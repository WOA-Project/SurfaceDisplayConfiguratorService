#pragma once

#include "Transition\CapturedDesktop.h"
#include "Transition\CompositionHost.h"

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

class SurfaceFactory
{
public:
    HRESULT CreatePanelVisual(CompositionHost &Host, const CapturedPanel &Panel, PanelVisual &Result);

private:
    HRESULT
    CreateSurfaceFromTexture(CompositionHost &Host, ID3D11Texture2D *SourceTexture, IDCompositionSurface **Surface);

    HRESULT
    CreateDebugSurface(
        CompositionHost &Host,
        UINT Width,
        UINT Height,
        D2D1_COLOR_F Color,
        IDCompositionSurface **Surface);
};

} // namespace Transition