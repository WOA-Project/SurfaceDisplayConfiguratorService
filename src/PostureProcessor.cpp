#include "pch.h"

#include "PostureProcessor.h"

#include "AutoRotateContext.h"

#include "OrientationManager.h"
#include "DisplayRotationManager.h"

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

    //
    // Full hinge angle.
    // Only one screen active.
    //
    if (Reading.HingeState() == Windows::Internal::System::HingeState::Full)
    {
        Display1State = g_AutoRotateContext.IsDisplay1SingleScreenFavorite;

        Display2State = !g_AutoRotateContext.IsDisplay1SingleScreenFavorite;
    }

    return SetDisplayStates(
        Panel1Id,
        Panel2Id,

        g_AutoRotateContext.Panel1Orientation,
        g_AutoRotateContext.Panel2Orientation,

        Display1State,
        Display2State);
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

    ApplyPostureReading(Reading);

    LeaveCriticalSection(&g_AutoRotateContext.CriticalSection);
}