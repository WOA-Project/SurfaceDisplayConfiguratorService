// FlipPreviewOverlay.h

#pragma once

#include <Windows.h>

HRESULT
ShowFlipPreviewOverlay(PCWSTR DisplayName);

void
HideFlipPreviewOverlay();

void
ForceHideFlipPreviewOverlay();

bool
IsFlipPreviewOverlayVisible();