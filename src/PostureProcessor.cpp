#include "pch.h"

#include "PostureProcessor.h"

#include "AutoRotateContext.h"

#include "OrientationManager.h"
#include "DisplayRotationManager.h"
#include "FlipConfirmationManager.h"

static bool s_LastWasFullHinge = false;

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

    BOOLEAN Display1State = TRUE;
    BOOLEAN Display2State = TRUE;

    BuildDisplayOrientations(
        g_AutoRotateContext.AutoRotationEnabled,
        Reading,
        &g_AutoRotateContext.Panel1Orientation,
        &g_AutoRotateContext.Panel2Orientation);

    bool IsFullHinge = Reading.HingeState() == Windows::Internal::System::HingeState::Full;

    //
    // Entering Full posture
    //
    if (IsFullHinge && !s_LastWasFullHinge)
    {
        BOOLEAN FinalDisplay1State = g_AutoRotateContext.IsDisplay1SingleScreenFavorite;
        BOOLEAN FinalDisplay2State = !g_AutoRotateContext.IsDisplay1SingleScreenFavorite;

        s_LastWasFullHinge = true;

        BeginPendingFlip(
            Panel1Id,
            Panel2Id,
            g_AutoRotateContext.Panel1Orientation,
            g_AutoRotateContext.Panel2Orientation,
            FinalDisplay1State,
            FinalDisplay2State);
    }

    //
    // Remaining in Full posture.
    //
    if (IsFullHinge)
    {
        //
        // Do not keep reapplying topology.
        // Let the pending flip manager own the transition.
        //
        return ERROR_SUCCESS;
    }

    //
    // Leaving Full posture.
    //
    if (!IsFullHinge && s_LastWasFullHinge)
    {
        CancelPendingFlip();

        s_LastWasFullHinge = false;
    }

    //
    // Normal dual-screen operation.
    //
    return SetDisplayStates(
        Panel1Id,
        Panel2Id,
        g_AutoRotateContext.Panel1Orientation,
        g_AutoRotateContext.Panel2Orientation,
        Display1State,
        Display2State,
        DISPLAY_PRIMARY_OVERRIDE::DisplayPrimaryAuto);
}

VOID WINAPI
ProcessCurrentPosture(TwoPanelHingedDevicePostureReading Reading)
{
    EnterCriticalSection(&g_AutoRotateContext.CriticalSection);

    ApplyPostureReading(Reading);

    LeaveCriticalSection(&g_AutoRotateContext.CriticalSection);
}

VOID WINAPI
TogglePreferredSingleScreen(TwoPanelHingedDevicePostureReading Reading)
{
    EnterCriticalSection(&g_AutoRotateContext.CriticalSection);

    g_AutoRotateContext.IsDisplay1SingleScreenFavorite = !g_AutoRotateContext.IsDisplay1SingleScreenFavorite;

    //
    // If we're currently folded,
    // restart the preview process immediately.
    //
    if (s_LastWasFullHinge)
    {
        CancelPendingFlip();

        s_LastWasFullHinge = false;
    }

    ApplyPostureReading(Reading);

    LeaveCriticalSection(&g_AutoRotateContext.CriticalSection);
}