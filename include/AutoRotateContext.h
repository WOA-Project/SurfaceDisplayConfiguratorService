#pragma once

#include <windows.h>

struct AutoRotateContext
{
    BOOLEAN AutoRotationEnabled;

    INT Panel1Orientation;
    INT Panel2Orientation;

    BOOLEAN IsDisplay1SingleScreenFavorite;

    CRITICAL_SECTION CriticalSection;
};

extern AutoRotateContext g_AutoRotateContext;

VOID WINAPI
InitializeAutoRotateContext();

VOID WINAPI
DestroyAutoRotateContext();