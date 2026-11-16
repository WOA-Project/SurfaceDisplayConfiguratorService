#include "pch.h"
#include "DisplayTopology.h"
#include <DisplayIdentityCache.h>
#include <DisplayRotationManager.h>
#include "DisplayValidation.h"

#include <atomic>
#include <DisplaySynchronization.h>

AppliedDisplayState g_DesiredState{};

std::atomic<BOOLEAN> g_RepairInProgress{false};
std::atomic<ULONGLONG> g_LastRepairTick{0};
std::atomic_bool g_SuppressValidation = false;

std::atomic<ULONGLONG> g_LastPolicyEvaluationTick{0};
std::atomic<BOOLEAN> g_ImmediateRepairRunning{false};

VOID
SaveDesiredStateLocked(
    BOOL NoOp,
    CONST WCHAR *LeftDisplayPanelId,
    CONST WCHAR *RightDisplayPanelId,
    INT LeftDisplayOrientation,
    INT RightDisplayOrientation,
    const DEVMODE &LeftDevMode,
    const DEVMODE &RightDevMode,
    BOOLEAN LeftDisplayState,
    BOOLEAN RightDisplayState,
    DISPLAY_PRIMARY_OVERRIDE PrimaryOverride)
{
    g_DesiredState.LeftPanelId = LeftDisplayPanelId;
    g_DesiredState.RightPanelId = RightDisplayPanelId;
    g_DesiredState.LeftOrientation = LeftDisplayOrientation;
    g_DesiredState.RightOrientation = RightDisplayOrientation;
    g_DesiredState.LeftEnabled = LeftDisplayState;
    g_DesiredState.RightEnabled = RightDisplayState;
    g_DesiredState.PrimaryOverride = PrimaryOverride;

    if (!NoOp)
    {
        g_DesiredState.LeftX = LeftDevMode.dmPosition.x;
        g_DesiredState.LeftY = LeftDevMode.dmPosition.y;
        g_DesiredState.RightX = RightDevMode.dmPosition.x;
        g_DesiredState.RightY = RightDevMode.dmPosition.y;

        g_DesiredState.LeftWidth = LeftDevMode.dmPelsWidth;
        g_DesiredState.LeftHeight = LeftDevMode.dmPelsHeight;

        g_DesiredState.RightWidth = RightDevMode.dmPelsWidth;
        g_DesiredState.RightHeight = RightDevMode.dmPelsHeight;
    }

    g_DesiredState.Valid = TRUE;
}

VOID
SaveDesiredState(
    BOOL NoOp,
    CONST WCHAR *LeftDisplayPanelId,
    CONST WCHAR *RightDisplayPanelId,
    INT LeftDisplayOrientation,
    INT RightDisplayOrientation,
    const DEVMODE &LeftDevMode,
    const DEVMODE &RightDevMode,
    BOOLEAN LeftDisplayState,
    BOOLEAN RightDisplayState,
    DISPLAY_PRIMARY_OVERRIDE PrimaryOverride)
{
    EnterCriticalSection(&g_DisplayOperationLock);

    SaveDesiredStateLocked(
        NoOp,
        LeftDisplayPanelId,
        RightDisplayPanelId,
        LeftDisplayOrientation,
        RightDisplayOrientation,
        LeftDevMode,
        RightDevMode,
        LeftDisplayState,
        RightDisplayState,
        PrimaryOverride);

    LeaveCriticalSection(&g_DisplayOperationLock);
}

BOOLEAN
ReadCurrentDisplayState(const WCHAR *LeftPanelId, const WCHAR *RightPanelId, CurrentDisplayState *State)
{
    if (!State)
    {
        return FALSE;
    }

    const DisplayIdentity *LeftIdentity = g_DisplayIdentityCache.FindByPanelId(LeftPanelId);

    const DisplayIdentity *RightIdentity = g_DisplayIdentityCache.FindByPanelId(RightPanelId);

    if (!LeftIdentity || !RightIdentity)
    {
        return FALSE;
    }

    DISPLAY_DEVICE LeftDisplay{};
    DISPLAY_DEVICE RightDisplay{};

    LeftDisplay.cb = sizeof(DISPLAY_DEVICE);
    RightDisplay.cb = sizeof(DISPLAY_DEVICE);

    HRESULT hr = GetDisplayDeviceByPanelId(LeftPanelId, &LeftDisplay);

    if (FAILED(hr))
    {
        return FALSE;
    }

    hr = GetDisplayDeviceByPanelId(RightPanelId, &RightDisplay);

    if (FAILED(hr))
    {
        return FALSE;
    }

    State->LeftEnabled = (LeftDisplay.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0;
    State->RightEnabled = (RightDisplay.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0;

    DEVMODE LeftMode{};
    LeftMode.dmSize = sizeof(LeftMode);

    DEVMODE RightMode{};
    RightMode.dmSize = sizeof(RightMode);

    if (EnumDisplaySettingsExW(LeftDisplay.DeviceName, ENUM_CURRENT_SETTINGS, &LeftMode, 0))
    {
        State->LeftOrientation = LeftMode.dmDisplayOrientation;
        State->LeftX = LeftMode.dmPosition.x;
        State->LeftY = LeftMode.dmPosition.y;
        State->LeftWidth = LeftMode.dmPelsWidth;
        State->LeftHeight = LeftMode.dmPelsHeight;
    }

    if (EnumDisplaySettingsExW(RightDisplay.DeviceName, ENUM_CURRENT_SETTINGS, &RightMode, 0))
    {
        State->RightOrientation = RightMode.dmDisplayOrientation;
        State->RightX = RightMode.dmPosition.x;
        State->RightY = RightMode.dmPosition.y;
        State->RightWidth = RightMode.dmPelsWidth;
        State->RightHeight = RightMode.dmPelsHeight;
    }

    wprintf(
        L"ReadCurrentDisplayState: "
        L"Left=%s Orient=%u Enabled=%u "
        L"Right=%s Orient=%u Enabled=%u\n",
        LeftDisplay.DeviceName,
        State->LeftOrientation,
        State->LeftEnabled,
        RightDisplay.DeviceName,
        State->RightOrientation,
        State->RightEnabled);

    return TRUE;
}

VOID
ValidateDisplayConfiguration()
{
    AppliedDisplayState DesiredCopy;

    if (g_RepairInProgress.load())
    {
        return;
    }

    {
        EnterCriticalSection(&g_DisplayOperationLock);

        DesiredCopy = g_DesiredState;

        LeaveCriticalSection(&g_DisplayOperationLock);
    }

    if (!DesiredCopy.Valid)
    {
        return;
    }

    if (g_SuppressValidation)
    {
        return;
    }

    if (GetTickCount64() - g_LastPolicyEvaluationTick.load() < 3000)
    {
        return;
    }

    BOOLEAN RepairRequired = FALSE;

    //
    // Query actual current display state.
    //
    CurrentDisplayState Actual{};

    if (!ReadCurrentDisplayState(DesiredCopy.LeftPanelId.c_str(), DesiredCopy.RightPanelId.c_str(), &Actual))
    {
        return;
    }

    if (Actual.LeftOrientation != DesiredCopy.LeftOrientation)
    {
        wprintf(L"DRIFT LeftOrientation expected=%d actual=%d\n", DesiredCopy.LeftOrientation, Actual.LeftOrientation);

        RepairRequired = TRUE;
    }

    if (Actual.RightOrientation != DesiredCopy.RightOrientation)
    {
        wprintf(
            L"DRIFT RightOrientation expected=%d actual=%d\n", DesiredCopy.RightOrientation, Actual.RightOrientation);

        RepairRequired = TRUE;
    }

    if (Actual.LeftEnabled != DesiredCopy.LeftEnabled)
    {
        wprintf(L"DRIFT LeftEnabled expected=%d actual=%d\n", DesiredCopy.LeftEnabled, Actual.LeftEnabled);

        RepairRequired = TRUE;
    }

    if (Actual.RightEnabled != DesiredCopy.RightEnabled)
    {
        wprintf(L"DRIFT RightEnabled expected=%d actual=%d\n", DesiredCopy.RightEnabled, Actual.RightEnabled);

        RepairRequired = TRUE;
    }

    if (Actual.LeftX != DesiredCopy.LeftX)
    {
        wprintf(L"DRIFT LeftX expected=%d actual=%d\n", DesiredCopy.LeftX, Actual.LeftX);

        RepairRequired = TRUE;
    }

    if (Actual.LeftY != DesiredCopy.LeftY)
    {
        wprintf(L"DRIFT LeftY expected=%d actual=%d\n", DesiredCopy.LeftY, Actual.LeftY);

        RepairRequired = TRUE;
    }

    if (Actual.RightX != DesiredCopy.RightX)
    {
        wprintf(L"DRIFT RightX expected=%d actual=%d\n", DesiredCopy.RightX, Actual.RightX);

        RepairRequired = TRUE;
    }

    if (Actual.RightY != DesiredCopy.RightY)
    {
        wprintf(L"DRIFT RightY expected=%d actual=%d\n", DesiredCopy.RightY, Actual.RightY);

        RepairRequired = TRUE;
    }

    if (Actual.LeftWidth != DesiredCopy.LeftWidth)
    {
        wprintf(L"DRIFT LeftWidth expected=%d actual=%d\n", DesiredCopy.LeftWidth, Actual.LeftWidth);

        RepairRequired = TRUE;
    }

    if (Actual.LeftHeight != DesiredCopy.LeftHeight)
    {
        wprintf(L"DRIFT LeftHeight expected=%d actual=%d\n", DesiredCopy.LeftHeight, Actual.LeftHeight);

        RepairRequired = TRUE;
    }

    if (Actual.RightWidth != DesiredCopy.RightWidth)
    {
        wprintf(L"DRIFT RightWidth expected=%d actual=%d\n", DesiredCopy.RightWidth, Actual.RightWidth);

        RepairRequired = TRUE;
    }

    if (Actual.RightHeight != DesiredCopy.RightHeight)
    {
        wprintf(L"DRIFT RightHeight expected=%d actual=%d\n", DesiredCopy.RightHeight, Actual.RightHeight);

        RepairRequired = TRUE;
    }

    if (!RepairRequired)
    {
        return;
    }

    wprintf(L"Display drift detected, scheduling repair\n");

    wprintf(
        L"Expected: "
        L"L=%d R=%d Enabled=(%d,%d)\n",
        DesiredCopy.LeftOrientation,
        DesiredCopy.RightOrientation,
        DesiredCopy.LeftEnabled,
        DesiredCopy.RightEnabled);

    wprintf(
        L"Actual: "
        L"L=%d R=%d Enabled=(%d,%d)\n",
        Actual.LeftOrientation,
        Actual.RightOrientation,
        Actual.LeftEnabled,
        Actual.RightEnabled);

    wprintf(
        L"Expected Topology: "
        L"L=(%ld,%ld) "
        L"R=(%ld,%ld)\n",
        DesiredCopy.LeftX,
        DesiredCopy.LeftY,
        DesiredCopy.RightX,
        DesiredCopy.RightY);

    wprintf(
        L"Actual Topology: "
        L"L=(%ld,%ld) "
        L"R=(%ld,%ld)\n",
        Actual.LeftX,
        Actual.LeftY,
        Actual.RightX,
        Actual.RightY);

    RepairDisplayConfiguration();
}

VOID
RepairDisplayConfiguration()
{
    BOOLEAN Expected = FALSE;

    if (!g_RepairInProgress.compare_exchange_strong(Expected, TRUE))
    {
        return;
    }

    ULONGLONG Now = GetTickCount64();

    if (Now - g_LastRepairTick.load() < 5000)
    {
        g_RepairInProgress.store(FALSE);
        return;
    }

    g_LastRepairTick.store(Now);

    AppliedDisplayState DesiredCopy;

    {
        EnterCriticalSection(&g_DisplayOperationLock);

        DesiredCopy = g_DesiredState;

        LeaveCriticalSection(&g_DisplayOperationLock);
    }

    CurrentDisplayState Actual{};

    if (ReadCurrentDisplayState(DesiredCopy.LeftPanelId.c_str(), DesiredCopy.RightPanelId.c_str(), &Actual))
    {
        wprintf(
            L"ACTUAL DURING REPAIR: "
            L"L=%d R=%d\n",
            Actual.LeftOrientation,
            Actual.RightOrientation);
    }

    wprintf(
        L"REPAIR REQUESTED: "
        L"L=%d R=%d\n",
        DesiredCopy.LeftOrientation,
        DesiredCopy.RightOrientation);

    wprintf(
        L"Display configuration drift detected. "
        L"Reapplying last known good state.\n");

    HRESULT hr = SetDisplayStatesInternal(
        DesiredCopy.LeftPanelId.c_str(),
        DesiredCopy.RightPanelId.c_str(),
        DesiredCopy.LeftOrientation,
        DesiredCopy.RightOrientation,
        DesiredCopy.LeftEnabled,
        DesiredCopy.RightEnabled,
        DesiredCopy.PrimaryOverride);

    wprintf(L"Repair result: 0x%08X\n", hr);

    g_RepairInProgress.store(FALSE);
}

BOOLEAN
IsConfigurationSatisfied(
    const WCHAR *LeftPanelId,
    const WCHAR *RightPanelId,
    INT LeftOrientation,
    INT RightOrientation,
    BOOLEAN LeftEnabled,
    BOOLEAN RightEnabled,
    LONG LeftX,
    LONG LeftY,
    LONG RightX,
    LONG RightY,
    ULONG LeftWidth,
    ULONG LeftHeight,
    ULONG RightWidth,
    ULONG RightHeight)
{
    CurrentDisplayState Actual{};

    if (!ReadCurrentDisplayState(LeftPanelId, RightPanelId, &Actual))
    {
        return FALSE;
    }

    return Actual.LeftOrientation == LeftOrientation && Actual.RightOrientation == RightOrientation &&
           Actual.LeftEnabled == LeftEnabled && Actual.RightEnabled == RightEnabled && Actual.LeftX == LeftX &&
           Actual.LeftY == LeftY && Actual.RightX == RightX && Actual.RightY == RightY &&
           Actual.LeftWidth == LeftWidth && Actual.LeftHeight == LeftHeight && Actual.RightWidth == RightWidth &&
           Actual.RightHeight == RightHeight;
}