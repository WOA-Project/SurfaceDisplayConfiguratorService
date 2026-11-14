#pragma once

#include <Windows.h>
#include <atomic>
#include <string>

#include <DisplayIdentityCache.h>
#include <DisplayRotationManager.h>

#include "DisplayTopology.h"

struct AppliedDisplayState
{
    std::wstring LeftPanelId;
    std::wstring RightPanelId;

    INT LeftOrientation;
    INT RightOrientation;

    BOOLEAN LeftEnabled;
    BOOLEAN RightEnabled;

    LONG LeftX;
    LONG LeftY;

    LONG RightX;
    LONG RightY;

    ULONG LeftWidth;
    ULONG LeftHeight;

    ULONG RightWidth;
    ULONG RightHeight;

    DISPLAY_PRIMARY_OVERRIDE PrimaryOverride;

    BOOLEAN Valid;
};

struct CurrentDisplayState
{
    INT LeftOrientation = DMDO_DEFAULT;
    INT RightOrientation = DMDO_DEFAULT;

    BOOLEAN LeftEnabled = FALSE;
    BOOLEAN RightEnabled = FALSE;

    LONG LeftX = 0;
    LONG LeftY = 0;

    LONG RightX = 0;
    LONG RightY = 0;

    ULONG LeftWidth = 0;
    ULONG LeftHeight = 0;

    ULONG RightWidth = 0;
    ULONG RightHeight = 0;
};

extern AppliedDisplayState g_DesiredState;

extern std::atomic<BOOLEAN> g_RepairInProgress;
extern std::atomic<ULONGLONG> g_LastRepairTick;
extern std::atomic_bool g_SuppressValidation;

extern std::atomic<ULONGLONG> g_LastPolicyEvaluationTick;
extern std::atomic<BOOLEAN> g_ImmediateRepairRunning;

VOID SaveDesiredState(
    BOOL NoOp,
    CONST WCHAR *LeftDisplayPanelId,
    CONST WCHAR *RightDisplayPanelId,
    INT LeftDisplayOrientation,
    INT RightDisplayOrientation,
    const DEVMODE& LeftDevMode,
    const DEVMODE& RightDevMode,
    BOOLEAN LeftDisplayState,
    BOOLEAN RightDisplayState,
    DISPLAY_PRIMARY_OVERRIDE PrimaryOverride);

VOID SaveDesiredStateLocked(
    BOOL NoOp,
    CONST WCHAR *LeftDisplayPanelId,
    CONST WCHAR *RightDisplayPanelId,
    INT LeftDisplayOrientation,
    INT RightDisplayOrientation,
    const DEVMODE& LeftDevMode,
    const DEVMODE& RightDevMode,
    BOOLEAN LeftDisplayState,
    BOOLEAN RightDisplayState,
    DISPLAY_PRIMARY_OVERRIDE PrimaryOverride);

BOOLEAN ReadCurrentDisplayState(
    const WCHAR *LeftPanelId,
    const WCHAR *RightPanelId,
    CurrentDisplayState *State);

BOOLEAN IsConfigurationSatisfied(
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
    ULONG RightHeight);

BOOLEAN VerifyDisplayConfiguration(
    CONST WCHAR *LeftDisplayPanelId,
    CONST WCHAR *RightDisplayPanelId,
    const DISPLAY_DEVICE& LeftDisplayDevice,
    const DISPLAY_DEVICE& RightDisplayDevice,
    INT LeftDisplayOrientation,
    INT RightDisplayOrientation,
    const DEVMODE& LeftDevMode,
    const DEVMODE& RightDevMode);

VOID ValidateDisplayConfiguration();

VOID RepairDisplayConfiguration();

VOID
VerifyPreDisplayConfiguration(CONST WCHAR *LeftDisplayPanelId, CONST WCHAR *RightDisplayPanelId);