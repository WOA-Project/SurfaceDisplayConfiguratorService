#include "pch.h"

#include "PostureProcessor.h"

#include "AutoRotateContext.h"

#include "OrientationManager.h"
#include "DisplayRotationManager.h"
#include "FlipConfirmationManager.h"

static bool s_InSingleScreenMode = false;

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

    g_AutoRotateContext.Panel1Orientation = Policy.Panel1Orientation;
    g_AutoRotateContext.Panel2Orientation = Policy.Panel2Orientation;

    bool IsSingleScreen = (Policy.Display1Enabled != Policy.Display2Enabled);

    //
    // Entering single-screen posture.
    //
    if (IsSingleScreen && !s_InSingleScreenMode)
    {
        s_InSingleScreenMode = true;

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
    if (!IsSingleScreen && s_InSingleScreenMode)
    {
        CancelPendingFlip();

        s_InSingleScreenMode = false;
    }

    //
    // Remaining single-screen.
    //
    if (IsSingleScreen && s_InSingleScreenMode)
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
    if (s_InSingleScreenMode)
    {
        CancelPendingFlip();

        s_InSingleScreenMode = false;
    }

    ApplyPostureReading(Reading);

    LeaveCriticalSection(&g_AutoRotateContext.CriticalSection);
}