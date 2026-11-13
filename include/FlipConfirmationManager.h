#pragma once

struct PendingFlip
{
    bool Active = false;

    WCHAR DisplayPanelId1[128];
    WCHAR DisplayPanelId2[128];

    INT DisplayOrientation1 = 0;
    INT DisplayOrientation2 = 0;

    BOOLEAN DisplayState1 = FALSE;
    BOOLEAN DisplayState2 = FALSE;
};

void BeginPendingFlip(
    CONST WCHAR *DisplayPanelId1,
    CONST WCHAR *DisplayPanelId2,
    INT DisplayOrientation1,
    INT DisplayOrientation2,
    BOOLEAN DisplayState1,
    BOOLEAN DisplayState2);

void CancelPendingFlip();

bool HasPendingFlip();

bool IsFlipPreviewActive();
void MarkFlipCommitted();

void
TogglePendingFlipPreview();