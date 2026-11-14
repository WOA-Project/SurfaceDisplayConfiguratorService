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
VerifyPreDisplayConfiguration(CONST WCHAR *LeftDisplayPanelId, CONST WCHAR *RightDisplayPanelId)
{
    DISPLAY_DEVICE VerifyLeft{};
    DISPLAY_DEVICE VerifyRight{};

    VerifyLeft.cb = sizeof(DISPLAY_DEVICE);
    VerifyRight.cb = sizeof(DISPLAY_DEVICE);

    HRESULT VerifyStatus;
    VerifyStatus = GetDisplayDeviceByPanelId(LeftDisplayPanelId, &VerifyLeft);
    if (SUCCEEDED(VerifyStatus))
    {
        wprintf(
            L"AFTER HW SYNC LEFT: %s Flags=0x%08X Attached=%d\n",
            VerifyLeft.DeviceName,
            VerifyLeft.StateFlags,
            (VerifyLeft.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0);
    }

    VerifyStatus = GetDisplayDeviceByPanelId(RightDisplayPanelId, &VerifyRight);
    if (SUCCEEDED(VerifyStatus))
    {
        wprintf(
            L"AFTER HW SYNC RIGHT: %s Flags=0x%08X Attached=%d\n",
            VerifyRight.DeviceName,
            VerifyRight.StateFlags,
            (VerifyRight.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0);
    }

    if ((VerifyLeft.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0 &&
        (VerifyRight.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0)
    {
        wprintf(L"!!! BOTH DISPLAYS DETACHED AFTER HW SYNC !!!\n");
    }
}

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

BOOLEAN
VerifyDisplayConfiguration(
    CONST WCHAR *LeftDisplayPanelId,
    CONST WCHAR *RightDisplayPanelId,
    const DISPLAY_DEVICE &LeftDisplayDevice,
    const DISPLAY_DEVICE &RightDisplayDevice,
    INT LeftDisplayOrientation,
    INT RightDisplayOrientation,
    const DEVMODE &LeftDevMode,
    const DEVMODE &RightDevMode)
{
    DISPLAY_DEVICE LeftVerify{};
    DISPLAY_DEVICE RightVerify{};
    BOOLEAN LeftAttached = FALSE;
    BOOLEAN RightAttached = FALSE;
    DEVMODE VerifyMode{};
    BOOLEAN ImmediateDrift = FALSE;
    DISPLAY_DEVICE FinalLeft{};
    DISPLAY_DEVICE FinalRight{};
    CurrentDisplayState Actual{};

    LeftVerify.cb = sizeof(DISPLAY_DEVICE);
    RightVerify.cb = sizeof(DISPLAY_DEVICE);

    GetDisplayDeviceByPanelId(LeftDisplayPanelId, &LeftVerify);
    GetDisplayDeviceByPanelId(RightDisplayPanelId, &RightVerify);

    VerifyMode.dmSize = sizeof(VerifyMode);

    EnumDisplaySettingsExW(LeftDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &VerifyMode, 0);

    wprintf(
        L"POST APPLY PRE SLEEP LEFT ORIENT=%u POS=(%ld,%ld)\n",
        VerifyMode.dmDisplayOrientation,
        VerifyMode.dmPosition.x,
        VerifyMode.dmPosition.y);

    EnumDisplaySettingsExW(RightDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &VerifyMode, 0);

    wprintf(
        L"POST APPLY PRE SLEEP RIGHT ORIENT=%u POS=(%ld,%ld)\n",
        VerifyMode.dmDisplayOrientation,
        VerifyMode.dmPosition.x,
        VerifyMode.dmPosition.y);

    wprintf(L"POST-COMMIT FLAGS: Left=0x%08X Right=0x%08X\n", LeftVerify.StateFlags, RightVerify.StateFlags);

    LeftAttached = (LeftVerify.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0;
    RightAttached = (RightVerify.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0;

    wprintf(L"POST-COMMIT ATTACHED: Left=%d Right=%d\n", LeftAttached, RightAttached);

    if (!LeftAttached && !RightAttached)
    {
        wprintf(L"!!! NO ACTIVE DISPLAYS AFTER COMMIT !!!\n");
    }

    wprintf(L"Sleeping after commit...\n");

    // Wait a second to make sure the display configuration is switched
    Sleep(1000);

    wprintf(L"Awake after sleep\n");

    VerifyMode.dmSize = sizeof(VerifyMode);

    EnumDisplaySettingsExW(LeftDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &VerifyMode, 0);

    wprintf(
        L"POST APPLY SLEEP LEFT ORIENT=%u POS=(%ld,%ld)\n",
        VerifyMode.dmDisplayOrientation,
        VerifyMode.dmPosition.x,
        VerifyMode.dmPosition.y);

    EnumDisplaySettingsExW(RightDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &VerifyMode, 0);

    wprintf(
        L"POST APPLY SLEEP RIGHT ORIENT=%u POS=(%ld,%ld)\n",
        VerifyMode.dmDisplayOrientation,
        VerifyMode.dmPosition.x,
        VerifyMode.dmPosition.y);

    if (ReadCurrentDisplayState(LeftDisplayPanelId, RightDisplayPanelId, &Actual))
    {
        wprintf(
            L"IMMEDIATE VERIFY:\n"
            L"  Expected L=%d R=%d\n"
            L"  Actual   L=%d R=%d\n",
            LeftDisplayOrientation,
            RightDisplayOrientation,
            Actual.LeftOrientation,
            Actual.RightOrientation);

        if (Actual.LeftOrientation != LeftDisplayOrientation || Actual.RightOrientation != RightDisplayOrientation)
        {
            wprintf(L"!!! IMMEDIATE ORIENTATION DRIFT DETECTED !!!\n");

            if (Actual.LeftOrientation != LeftDisplayOrientation)
            {
                wprintf(
                    L"DRIFT LEFT ORIENTATION: "
                    L"expected=%d actual=%d\n",
                    LeftDisplayOrientation,
                    Actual.LeftOrientation);
            }

            if (Actual.RightOrientation != RightDisplayOrientation)
            {
                wprintf(
                    L"DRIFT RIGHT ORIENTATION: "
                    L"expected=%d actual=%d\n",
                    RightDisplayOrientation,
                    Actual.RightOrientation);
            }

            ImmediateDrift = TRUE;
        }

        if (Actual.LeftX != LeftDevMode.dmPosition.x || Actual.LeftY != LeftDevMode.dmPosition.y ||
            Actual.RightX != RightDevMode.dmPosition.x || Actual.RightY != RightDevMode.dmPosition.y)
        {
            wprintf(L"!!! IMMEDIATE TOPOLOGY DRIFT DETECTED !!!\n");

            ImmediateDrift = TRUE;
        }

        if (Actual.LeftWidth != LeftDevMode.dmPelsWidth || Actual.LeftHeight != LeftDevMode.dmPelsHeight ||
            Actual.RightWidth != RightDevMode.dmPelsWidth || Actual.RightHeight != RightDevMode.dmPelsHeight)
        {
            wprintf(L"!!! IMMEDIATE SIZE DRIFT DETECTED !!!\n");
            ImmediateDrift = TRUE;
        }
    }

    FinalLeft.cb = sizeof(DISPLAY_DEVICE);
    FinalRight.cb = sizeof(DISPLAY_DEVICE);

    GetDisplayDeviceByPanelId(LeftDisplayPanelId, &FinalLeft);
    GetDisplayDeviceByPanelId(RightDisplayPanelId, &FinalRight);

    wprintf(L"AFTER SLEEP FLAGS: Left=0x%08X Right=0x%08X\n", FinalLeft.StateFlags, FinalRight.StateFlags);

    wprintf(
        L"AFTER SLEEP ATTACHED: Left=%d Right=%d\n",
        (FinalLeft.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0,
        (FinalRight.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0);

    return ImmediateDrift;
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