#include "pch.h"

#include "PostureProcessor.h"

#include "AutoRotateContext.h"

#include "OrientationManager.h"
#include "DisplayRotationManager.h"
#include "FlipConfirmationManager.h"
#include <AutoRotateService.h>
#include <DisplaySynchronization.h>

std::atomic<SingleScreenState> s_InSingleScreenMode{SingleScreenState::DualScreen};

HRESULT WINAPI
ApplyPostureReading(TwoPanelHingedDevicePostureReading Reading)
{
    const WCHAR *Panel1Id = Reading.Panel1Id().c_str();
    const WCHAR *Panel2Id = Reading.Panel2Id().c_str();

    wprintf(
        L"Posture API:\n"
        L"    Panel1Id=%s\n"
        L"    Panel2Id=%s\n",
        Panel1Id,
        Panel2Id);

    DisplayPolicy Policy = BuildDisplayPolicy(
        g_AutoRotateContext.AutoRotationEnabled, Reading, g_AutoRotateContext.IsDisplay1SingleScreenFavorite);

    wprintf(
        L"Policy to apply:\n"
        L"    Display1Enabled: %d\n"
        L"    Display2Enabled: %d\n"
        L"    Panel1Orientation: %d\n"
        L"    Panel2Orientation: %d\n",
        Policy.Display1Enabled,
        Policy.Display2Enabled,
        Policy.Panel1Orientation,
        Policy.Panel2Orientation);

    bool IsSingleScreen = (Policy.Display1Enabled != Policy.Display2Enabled);

    //
    // Entering single-screen posture.
    //
    if (IsSingleScreen && s_InSingleScreenMode.load() == SingleScreenState::DualScreen)
    {
        s_InSingleScreenMode.store(SingleScreenState::PreviewPending);

        BeginPendingFlip(
            Panel1Id,
            Panel2Id,
            Policy.Panel1Orientation,
            Policy.Panel2Orientation,
            Policy.Display1Enabled,
            Policy.Display2Enabled);

        return ERROR_SUCCESS;
    }

    //
    // Leaving single-screen mode.
    //
    if (!IsSingleScreen && s_InSingleScreenMode.load() != SingleScreenState::DualScreen)
    {
        CancelPendingFlip();
    }

    //
    // Remaining single-screen.
    //
    if (IsSingleScreen && s_InSingleScreenMode.load() != SingleScreenState::DualScreen)
    {
        return ERROR_SUCCESS;
    }

    //
    // Normal dual-screen operation.
    //
    return SetDisplayStates(
        Panel1Id,
        Panel2Id,
        Policy.Panel1Orientation,
        Policy.Panel2Orientation,
        Policy.Display1Enabled,
        Policy.Display2Enabled,
        DISPLAY_PRIMARY_OVERRIDE::DisplayPrimaryAuto);
}

VOID WINAPI
ProcessCurrentPosture(TwoPanelHingedDevicePostureReading Reading)
{
    EnterCriticalSection(&g_DisplayOperationLock);
    ApplyPostureReading(Reading);
    LeaveCriticalSection(&g_DisplayOperationLock);
}

VOID WINAPI
TogglePreferredSingleScreen()
{
    EnterCriticalSection(&g_DisplayOperationLock);
    g_AutoRotateContext.IsDisplay1SingleScreenFavorite = !g_AutoRotateContext.IsDisplay1SingleScreenFavorite;
    LeaveCriticalSection(&g_DisplayOperationLock);

    //
    // If we're currently folded,
    // restart the preview process immediately.
    //
    if (s_InSingleScreenMode.load() != SingleScreenState::DualScreen)
    {
        CancelPendingFlip();
    }

    AutoRotateService *Service = GetAutoRotateService();
    if (!Service)
    {
        return;
    }

    ApplyPostureReading(Service->GetSensorManager().GetCurrentPosture());
}