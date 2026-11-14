#include "pch.h"

#include "DisplayLayout.h"

const PANEL_EDGE PhysicalLeftHingeEdge = PanelEdgeRight;
const PANEL_EDGE PhysicalRightHingeEdge = PanelEdgeLeft;

static const WCHAR *
EdgeName(PANEL_EDGE Edge)
{
    switch (Edge)
    {
    case PanelEdgeLeft:
        return L"LEFT";

    case PanelEdgeRight:
        return L"RIGHT";

    case PanelEdgeTop:
        return L"TOP";

    case PanelEdgeBottom:
        return L"BOTTOM";
    }

    return L"?";
}

static PANEL_EDGE
Rotate90(PANEL_EDGE Edge)
{
    switch (Edge)
    {
    case PanelEdgeTop:
        return PanelEdgeRight;

    case PanelEdgeRight:
        return PanelEdgeBottom;

    case PanelEdgeBottom:
        return PanelEdgeLeft;

    case PanelEdgeLeft:
        return PanelEdgeTop;
    }

    return Edge;
}

PANEL_EDGE WINAPI
RotateEdge(PANEL_EDGE Edge, INT Orientation)
{
    PANEL_EDGE Result = Edge;

    switch (Orientation)
    {
    case DMDO_DEFAULT:
        break;

    case DMDO_90:
        Result = Rotate90(Result);
        break;

    case DMDO_180:
        Result = Rotate90(Rotate90(Result));
        break;

    case DMDO_270:
        Result = Rotate90(Rotate90(Rotate90(Result)));
        break;
    }

    return Result;
}

DISPLAY_CONNECTION WINAPI
DetermineDisplayConnection(INT LeftOrientation, INT RightOrientation)
{
    DISPLAY_CONNECTION Connection{};

    Connection.LeftEdge = RotateEdge(PhysicalLeftHingeEdge, LeftOrientation);

    Connection.RightEdge = RotateEdge(PhysicalRightHingeEdge, RightOrientation);

    wprintf(
        L"CONNECTION: "
        L"%s <-> %s\n",
        EdgeName(Connection.LeftEdge),
        EdgeName(Connection.RightEdge));

    return Connection;
}

VOID WINAPI
BuildExtendedDisplayLayout(const DISPLAY_CONNECTION &Connection, PDEVMODE LeftDevMode, PDEVMODE RightDevMode)
{
    //
    // Default origin.
    //
    LeftDevMode->dmPosition.x = 0;
    LeftDevMode->dmPosition.y = 0;

    RightDevMode->dmPosition.x = 0;
    RightDevMode->dmPosition.y = 0;

    //
    // RIGHT <-> LEFT
    //
    if (Connection.LeftEdge == PanelEdgeRight && Connection.RightEdge == PanelEdgeLeft)
    {
        RightDevMode->dmPosition.x = LeftDevMode->dmPelsWidth;
    }

    //
    // LEFT <-> RIGHT
    //
    else if (Connection.LeftEdge == PanelEdgeLeft && Connection.RightEdge == PanelEdgeRight)
    {
        //
        // Keep coordinates positive.
        //
        LeftDevMode->dmPosition.x = RightDevMode->dmPelsWidth;
    }

    //
    // BOTTOM <-> TOP
    //
    else if (Connection.LeftEdge == PanelEdgeBottom && Connection.RightEdge == PanelEdgeTop)
    {
        LeftDevMode->dmPosition.y = RightDevMode->dmPelsHeight;
    }

    //
    // TOP <-> BOTTOM
    //
    else if (Connection.LeftEdge == PanelEdgeTop && Connection.RightEdge == PanelEdgeBottom)
    {
        LeftDevMode->dmPosition.y = RightDevMode->dmPelsHeight;
    }

    //
    // TOP <-> TOP
    //
    else if (Connection.LeftEdge == PanelEdgeTop && Connection.RightEdge == PanelEdgeTop)
    {
        //
        // No uniquely correct answer.
        //
        // Choose a deterministic positive-coordinate
        // layout with the left display as origin.
        //
        RightDevMode->dmPosition.x = LeftDevMode->dmPelsWidth;
    }

    //
    // BOTTOM <-> BOTTOM
    //
    else if (Connection.LeftEdge == PanelEdgeBottom && Connection.RightEdge == PanelEdgeBottom)
    {
        RightDevMode->dmPosition.x = LeftDevMode->dmPelsWidth;
    }

    //
    // LEFT <-> LEFT
    //
    else if (Connection.LeftEdge == PanelEdgeLeft && Connection.RightEdge == PanelEdgeLeft)
    {
        RightDevMode->dmPosition.y = LeftDevMode->dmPelsHeight;
    }

    //
    // RIGHT <-> RIGHT
    //
    else if (Connection.LeftEdge == PanelEdgeRight && Connection.RightEdge == PanelEdgeRight)
    {
        RightDevMode->dmPosition.y = LeftDevMode->dmPelsHeight;
    }
    else
    {
        wprintf(
            L"WARNING: Unsupported connection "
            L"%s <-> %s\n",
            EdgeName(Connection.LeftEdge),
            EdgeName(Connection.RightEdge));

        RightDevMode->dmPosition.x = LeftDevMode->dmPelsWidth;
    }

    wprintf(
        L"LAYOUT: "
        L"%s <-> %s "
        L"L=(%ld,%ld) "
        L"R=(%ld,%ld)\n",
        EdgeName(Connection.LeftEdge),
        EdgeName(Connection.RightEdge),
        LeftDevMode->dmPosition.x,
        LeftDevMode->dmPosition.y,
        RightDevMode->dmPosition.x,
        RightDevMode->dmPosition.y);
}