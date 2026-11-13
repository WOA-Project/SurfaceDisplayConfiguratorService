#include "pch.h"

#include "FlipConfirmationManager.h"
#include "DisplayRotationManager.h"
#include "FlipPreviewOverlay.h"
#include <DisplayIdentityCache.h>
#include <DisplayTopology.h>

static PendingFlip g_PendingFlip{};

static HANDLE g_FlipTimer = nullptr;

static bool g_FlipPreviewActive = false;

static void
CommitPendingFlip()
{
    if (!g_PendingFlip.Active)
    {
        return;
    }

    wprintf(L"CommitPendingFlip: P1=%d P2=%d\n", g_PendingFlip.DisplayState1, g_PendingFlip.DisplayState2);

    HANDLE Timer = g_FlipTimer;

    g_FlipTimer = nullptr;
    g_PendingFlip.Active = false;
    g_FlipPreviewActive = false;

    HideFlipPreviewOverlay();

    SetDisplayStates(
        g_PendingFlip.DisplayPanelId1,
        g_PendingFlip.DisplayPanelId2,
        g_PendingFlip.DisplayOrientation1,
        g_PendingFlip.DisplayOrientation2,
        g_PendingFlip.DisplayState1,
        g_PendingFlip.DisplayState2,
        DisplayPrimaryAuto);

    if (Timer)
    {
        DeleteTimerQueueTimer(nullptr, Timer, INVALID_HANDLE_VALUE);
    }
}

static VOID CALLBACK
PendingFlipTimer(PVOID, BOOLEAN)
{
    CommitPendingFlip();
}

void
BeginPendingFlip(
    CONST WCHAR *DisplayPanelId1,
    CONST WCHAR *DisplayPanelId2,
    INT DisplayOrientation1,
    INT DisplayOrientation2,
    BOOLEAN DisplayState1,
    BOOLEAN DisplayState2)
{
    CancelPendingFlip();

    g_FlipPreviewActive = true;
    g_PendingFlip.Active = true;

    wcscpy_s(g_PendingFlip.DisplayPanelId1, DisplayPanelId1);
    wcscpy_s(g_PendingFlip.DisplayPanelId2, DisplayPanelId2);

    g_PendingFlip.DisplayOrientation1 = DisplayOrientation1;
    g_PendingFlip.DisplayOrientation2 = DisplayOrientation2;

    g_PendingFlip.DisplayState1 = DisplayState1;
    g_PendingFlip.DisplayState2 = DisplayState2;

    DISPLAY_PRIMARY_OVERRIDE PreviewPrimary =
        DisplayState1 ? DISPLAY_PRIMARY_OVERRIDE::DisplayPrimary1 : DISPLAY_PRIMARY_OVERRIDE::DisplayPrimary2;

    //
    // Preview mode:
    // always show BOTH displays
    //

    SetDisplayStates(
        DisplayPanelId1, DisplayPanelId2, DisplayOrientation1, DisplayOrientation2, TRUE, TRUE, PreviewPrimary);

    const DisplayIdentity *Panel1 = g_DisplayIdentityCache.FindByPanelId(DisplayPanelId1);
    const DisplayIdentity *Panel2 = g_DisplayIdentityCache.FindByPanelId(DisplayPanelId2);

    if (Panel1 && Panel2)
    {
        DISPLAY_DEVICE DisplayDevice1;
        DISPLAY_DEVICE DisplayDevice2;

        HRESULT Status = GetDisplayDeviceByPanelId(DisplayPanelId1, &DisplayDevice1);
        if (FAILED(Status))
        {
            return;
        }

        Status = GetDisplayDeviceByPanelId(DisplayPanelId2, &DisplayDevice2);
        if (FAILED(Status))
        {
            return;
        }

        if (PreviewPrimary == DisplayPrimary1)
        {
            //
            // Show overlay on WRONG display.
            //
            ShowFlipPreviewOverlay(DisplayDevice2.DeviceName);
        }
        else
        {
            ShowFlipPreviewOverlay(DisplayDevice1.DeviceName);
        }
    }

    CreateTimerQueueTimer(&g_FlipTimer, nullptr, PendingFlipTimer, nullptr, 15000, 0, WT_EXECUTEDEFAULT);
}

void
CancelPendingFlip()
{
    HideFlipPreviewOverlay();

    if (g_FlipTimer)
    {
        DeleteTimerQueueTimer(nullptr, g_FlipTimer, INVALID_HANDLE_VALUE);

        g_FlipTimer = nullptr;
    }

    g_PendingFlip.Active = false;
    g_FlipPreviewActive = false;
}

bool
HasPendingFlip()
{
    return g_PendingFlip.Active;
}

bool
IsFlipPreviewActive()
{
    return g_FlipPreviewActive;
}

void
MarkFlipCommitted()
{
    g_FlipPreviewActive = false;
}

void
TogglePendingFlipPreview()
{
    if (!g_PendingFlip.Active)
    {
        return;
    }

    std::swap(
        g_PendingFlip.DisplayState1,
        g_PendingFlip.DisplayState2);

    DISPLAY_PRIMARY_OVERRIDE PreviewPrimary =
        g_PendingFlip.DisplayState1
            ? DisplayPrimary1
            : DisplayPrimary2;

    SetDisplayStates(
        g_PendingFlip.DisplayPanelId1,
        g_PendingFlip.DisplayPanelId2,
        g_PendingFlip.DisplayOrientation1,
        g_PendingFlip.DisplayOrientation2,
        TRUE,
        TRUE,
        PreviewPrimary);

    DISPLAY_DEVICE DisplayDevice1;
    DISPLAY_DEVICE DisplayDevice2;

    if (SUCCEEDED(GetDisplayDeviceByPanelId(
            g_PendingFlip.DisplayPanelId1,
            &DisplayDevice1)) &&
        SUCCEEDED(GetDisplayDeviceByPanelId(
            g_PendingFlip.DisplayPanelId2,
            &DisplayDevice2)))
    {
        HideFlipPreviewOverlay();

        if (PreviewPrimary == DisplayPrimary1)
        {
            ShowFlipPreviewOverlay(
                DisplayDevice2.DeviceName);
        }
        else
        {
            ShowFlipPreviewOverlay(
                DisplayDevice1.DeviceName);
        }
    }
}