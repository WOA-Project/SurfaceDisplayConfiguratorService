#pragma once

#include <windows.h>

struct AutoRotateContext
{
    BOOLEAN AutoRotationEnabled;
    BOOLEAN IsDisplay1SingleScreenFavorite;
};

extern AutoRotateContext g_AutoRotateContext;

VOID WINAPI
InitializeAutoRotateContext();