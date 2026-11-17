#pragma once

#include "Transition\\CapturedDesktop.h"
#include "Transition\\CompositionHost.h"
#include "Transition\\PanelVisual.h"

#include <dcomp.h>
#include <d2d1.h>

namespace Transition {

class SurfaceFactory
{
public:
    HRESULT CreateDebugSurface(
        CompositionHost &Host,
        UINT Width,
        UINT Height,
        D2D1_COLOR_F Color,
        IDCompositionSurface **Surface);

    HRESULT
    CreateSurfaceFromTexture(CompositionHost &Host, ID3D11Texture2D *SourceTexture, IDCompositionSurface **Surface);

    HRESULT CreatePanelVisual(CompositionHost &Host, const CapturedPanel &Panel, PanelVisual &Result);
};

} // namespace Transition