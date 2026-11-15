#include "pch.h"

#include "FlipConfirmationManager.h"
#include "DisplayRotationManager.h"
#include "FlipPreviewOverlay.h"
#include <DisplayIdentityCache.h>
#include <DisplayTopology.h>
#include "AutoRotateContext.h"
#include <PostureProcessor.h>
#include <InputStitching\InputSpaceRuntime.h>

static PendingFlip g_PendingFlip{};
static HANDLE g_FlipTimer = nullptr;
static bool g_FlipPreviewActive = false;
CRITICAL_SECTION g_FlipLock;

struct FlipLockInitializer
{
    FlipLockInitializer() { InitializeCriticalSection(&g_FlipLock); }
    ~FlipLockInitializer() { DeleteCriticalSection(&g_FlipLock); }
};

static FlipLockInitializer g_FlipLockInitializer;

void
CancelPendingFlipLocked()
{
    g_PendingFlip.Active = false;
    g_FlipPreviewActive = false;
}

static void
CommitPendingFlip()
{
    PendingFlip flip{};
    HANDLE timer = nullptr;

    EnterCriticalSection(&g_FlipLock);

    if (!g_PendingFlip.Active)
    {
        LeaveCriticalSection(&g_FlipLock);
        return;
    }

    flip = g_PendingFlip;

    timer = g_FlipTimer;
    g_FlipTimer = nullptr;

    g_PendingFlip.Active = false;
    g_FlipPreviewActive = false;

    LeaveCriticalSection(&g_FlipLock);

    ForceHideFlipPreviewOverlay();

    HRESULT hr = SetDisplayStates(
        flip.DisplayPanelId1,
        flip.DisplayPanelId2,
        flip.DisplayOrientation1,
        flip.DisplayOrientation2,
        flip.DisplayState1,
        flip.DisplayState2,
        DisplayPrimaryAuto);

    if (FAILED(hr))
    {
        wprintf(L"Pending flip commit failed: 0x%08X\n", hr);
        s_InSingleScreenMode.store(SingleScreenState::DualScreen);
        return;
    }

    s_InSingleScreenMode.store(SingleScreenState::SingleScreenCommitted);

    if (timer)
    {
        DeleteTimerQueueTimer(nullptr, timer, INVALID_HANDLE_VALUE);
    }

    InputStitching::g_InputSpaceRuntime.Rebuild();
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
    HANDLE oldTimer = nullptr;

    EnterCriticalSection(&g_FlipLock);

    oldTimer = g_FlipTimer;
    g_FlipTimer = nullptr;

    CancelPendingFlipLocked();

    g_FlipPreviewActive = true;
    g_PendingFlip.Active = true;

    wcscpy_s(g_PendingFlip.DisplayPanelId1, DisplayPanelId1);
    wcscpy_s(g_PendingFlip.DisplayPanelId2, DisplayPanelId2);

    g_PendingFlip.DisplayOrientation1 = DisplayOrientation1;
    g_PendingFlip.DisplayOrientation2 = DisplayOrientation2;
    g_PendingFlip.DisplayState1 = DisplayState1;
    g_PendingFlip.DisplayState2 = DisplayState2;

    LeaveCriticalSection(&g_FlipLock);
    HideFlipPreviewOverlay();

    if (oldTimer)
    {
        DeleteTimerQueueTimer(nullptr, oldTimer, INVALID_HANDLE_VALUE);
    }

    DISPLAY_PRIMARY_OVERRIDE previewPrimary = DisplayState1 ? DisplayPrimary1 : DisplayPrimary2;

    HRESULT hr = SetDisplayStates(
        DisplayPanelId1, DisplayPanelId2, DisplayOrientation1, DisplayOrientation2, TRUE, TRUE, previewPrimary);
    if (FAILED(hr))
    {
        s_InSingleScreenMode.store(SingleScreenState::DualScreen);
    }

    DISPLAY_DEVICE displayDevice1{};
    DISPLAY_DEVICE displayDevice2{};

    displayDevice1.cb = sizeof(DISPLAY_DEVICE);
    displayDevice2.cb = sizeof(DISPLAY_DEVICE);

    HRESULT res1 = GetDisplayDeviceByPanelId(DisplayPanelId1, &displayDevice1);
    HRESULT res2 = GetDisplayDeviceByPanelId(DisplayPanelId2, &displayDevice2);
    if (SUCCEEDED(res1) && SUCCEEDED(res2))
    {
        if (DisplayState1)
        {
            //
            // Display1 will remain active.
            // Show confirmation on Display2.
            //
            ShowFlipPreviewOverlay(displayDevice2.DeviceName);
        }
        else
        {
            //
            // Display2 will remain active.
            // Show confirmation on Display1.
            //
            ShowFlipPreviewOverlay(displayDevice1.DeviceName);
        }
    }

    HANDLE newTimer = nullptr;
    if (!CreateTimerQueueTimer(&newTimer, nullptr, PendingFlipTimer, nullptr, 15000, 0, WT_EXECUTEDEFAULT))
    {
        wprintf(L"Failed to create flip timer: %u\n", GetLastError());
        EnterCriticalSection(&g_FlipLock);
        g_PendingFlip.Active = false;
        g_FlipPreviewActive = false;
        LeaveCriticalSection(&g_FlipLock);
        HideFlipPreviewOverlay();
        s_InSingleScreenMode.store(SingleScreenState::DualScreen);
        return;
    }

    EnterCriticalSection(&g_FlipLock);
    g_FlipTimer = newTimer;
    LeaveCriticalSection(&g_FlipLock);
}

void
CancelPendingFlip()
{
    HANDLE timer = nullptr;

    EnterCriticalSection(&g_FlipLock);

    timer = g_FlipTimer;
    g_FlipTimer = nullptr;

    CancelPendingFlipLocked();

    LeaveCriticalSection(&g_FlipLock);

    HideFlipPreviewOverlay();

    if (timer)
    {
        DeleteTimerQueueTimer(nullptr, timer, INVALID_HANDLE_VALUE);
    }

    s_InSingleScreenMode.store(SingleScreenState::DualScreen);
}

bool
HasPendingFlip()
{
    EnterCriticalSection(&g_FlipLock);
    bool active = g_PendingFlip.Active;
    LeaveCriticalSection(&g_FlipLock);
    return active;
}

bool
IsFlipPreviewActive()
{
    EnterCriticalSection(&g_FlipLock);
    bool active = g_FlipPreviewActive;
    LeaveCriticalSection(&g_FlipLock);
    return active;
}

void
MarkFlipCommitted()
{
    EnterCriticalSection(&g_FlipLock);
    g_FlipPreviewActive = false;
    LeaveCriticalSection(&g_FlipLock);
}

void
TogglePendingFlipPreview()
{
    PendingFlip flip{};
    DISPLAY_PRIMARY_OVERRIDE previewPrimary;

    EnterCriticalSection(&g_FlipLock);

    if (!g_PendingFlip.Active)
    {
        LeaveCriticalSection(&g_FlipLock);
        return;
    }

    std::swap(g_PendingFlip.DisplayState1, g_PendingFlip.DisplayState2);

    //
    // Persist user's preferred single-screen target.
    //
    g_AutoRotateContext.IsDisplay1SingleScreenFavorite = g_PendingFlip.DisplayState1;
    previewPrimary = g_PendingFlip.DisplayState1 ? DisplayPrimary1 : DisplayPrimary2;
    flip = g_PendingFlip;

    LeaveCriticalSection(&g_FlipLock);

    ForceHideFlipPreviewOverlay();

    SetDisplayStates(
        flip.DisplayPanelId1,
        flip.DisplayPanelId2,
        flip.DisplayOrientation1,
        flip.DisplayOrientation2,
        TRUE,
        TRUE,
        previewPrimary);

    DISPLAY_DEVICE displayDevice1{};
    DISPLAY_DEVICE displayDevice2{};

    displayDevice1.cb = sizeof(DISPLAY_DEVICE);
    displayDevice2.cb = sizeof(DISPLAY_DEVICE);

    HRESULT res1 = GetDisplayDeviceByPanelId(flip.DisplayPanelId1, &displayDevice1);
    HRESULT res2 = GetDisplayDeviceByPanelId(flip.DisplayPanelId2, &displayDevice2);
    if (SUCCEEDED(res1) && SUCCEEDED(res2))
    {
        if (flip.DisplayState1)
        {
            //
            // Display1 will remain active.
            // Show confirmation on Display2.
            //
            ShowFlipPreviewOverlay(displayDevice2.DeviceName);
        }
        else
        {
            //
            // Display2 will remain active.
            // Show confirmation on Display1.
            //
            ShowFlipPreviewOverlay(displayDevice1.DeviceName);
        }
    }
}