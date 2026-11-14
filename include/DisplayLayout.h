#pragma once

#include <windows.h>

typedef enum _PANEL_EDGE
{
    PanelEdgeLeft,
    PanelEdgeRight,
    PanelEdgeTop,
    PanelEdgeBottom
} PANEL_EDGE;

typedef struct _DISPLAY_CONNECTION
{
    PANEL_EDGE LeftEdge;
    PANEL_EDGE RightEdge;
} DISPLAY_CONNECTION;

PANEL_EDGE WINAPI
RotateEdge(
    PANEL_EDGE Edge,
    INT Orientation);

DISPLAY_CONNECTION WINAPI
DetermineDisplayConnection(
    INT LeftOrientation,
    INT RightOrientation);

VOID WINAPI
BuildExtendedDisplayLayout(
    const DISPLAY_CONNECTION& Connection,
    PDEVMODE LeftDevMode,
    PDEVMODE RightDevMode);