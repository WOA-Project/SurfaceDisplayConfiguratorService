#pragma once

#include <windows.h>

HRESULT WINAPI
ApplyExtendedDisplayConfiguration(
    PDISPLAY_DEVICE LeftDisplayDevice,
    PDISPLAY_DEVICE RightDisplayDevice,
    PDEVMODE LeftDevMode,
    PDEVMODE RightDevMode);

HRESULT WINAPI
ApplySingleLeftDisplayConfiguration(
    PDISPLAY_DEVICE LeftDisplayDevice,
    PDISPLAY_DEVICE RightDisplayDevice,
    PDEVMODE LeftDevMode,
    PDEVMODE RightDevMode,
    BOOLEAN LastRightDisplayState);

HRESULT WINAPI
ApplySingleRightDisplayConfiguration(
    PDISPLAY_DEVICE LeftDisplayDevice,
    PDISPLAY_DEVICE RightDisplayDevice,
    PDEVMODE LeftDevMode,
    PDEVMODE RightDevMode,
    BOOLEAN LastLeftDisplayState);

HRESULT WINAPI
CommitDisplayConfiguration();