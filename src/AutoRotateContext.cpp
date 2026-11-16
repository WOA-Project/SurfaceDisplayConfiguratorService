#include "pch.h"

#include "AutoRotateContext.h"

AutoRotateContext g_AutoRotateContext = {};

VOID WINAPI
InitializeAutoRotateContext()
{
    g_AutoRotateContext.AutoRotationEnabled = TRUE;
    g_AutoRotateContext.IsDisplay1SingleScreenFavorite = FALSE;
}