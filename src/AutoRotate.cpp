#include "pch.h"

#include "AutoRotate.h"

#include "AutoRotateService.h"

static AutoRotateService g_AutoRotateService;

VOID
AutoRotateMain()
{
    HRESULT Status = g_AutoRotateService.Initialize();

    if (FAILED(Status))
    {
        printf("AutoRotateService.Initialize failed: 0x%08X\n", Status);

        return;
    }

    g_AutoRotateService.Run();
}