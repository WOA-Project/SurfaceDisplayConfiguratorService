#include "pch.h"

#include "AutoRotateContext.h"

AutoRotateContext g_AutoRotateContext = {};

VOID WINAPI
InitializeAutoRotateContext()
{
    g_AutoRotateContext.AutoRotationEnabled = TRUE;

    g_AutoRotateContext.Panel1Orientation = 0;
    g_AutoRotateContext.Panel2Orientation = 0;

    g_AutoRotateContext.IsDisplay1SingleScreenFavorite = FALSE;
}