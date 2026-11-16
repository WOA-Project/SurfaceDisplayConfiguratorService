#pragma once

#include <windows.h>

struct AutoRotateContext
{
    BOOLEAN AutoRotationEnabled;

    INT Panel1Orientation;
    INT Panel2Orientation;

    BOOLEAN IsDisplay1SingleScreenFavorite;
};

extern AutoRotateContext g_AutoRotateContext;

VOID WINAPI
InitializeAutoRotateContext();