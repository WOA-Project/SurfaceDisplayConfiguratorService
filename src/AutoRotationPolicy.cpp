#include "pch.h"
#include "AutoRotationPolicy.h"
#include <AutoRotateContext.h>
#include <AutoRotateService.h>

static BOOLEAN g_DisabledAutoRotation = FALSE;
static BOOLEAN g_OriginalAutoRotationEnabled = FALSE;
static BOOLEAN g_HaveOriginalAutoRotationState = FALSE;

static CRITICAL_SECTION g_AutoRotationPolicyLock;

VOID
UpdateAutoRotationPolicy(INT LeftOrientation, INT RightOrientation, BOOLEAN LeftEnabled, BOOLEAN RightEnabled)
{
    EnterCriticalSection(&g_AutoRotationPolicyLock);

    BOOLEAN MixedOrientation = LeftEnabled && RightEnabled && LeftOrientation != RightOrientation;

    if (MixedOrientation)
    {
        if (!g_DisabledAutoRotation)
        {
            if (!g_HaveOriginalAutoRotationState)
            {
                EnterCriticalSection(&g_AutoRotateContext.CriticalSection);
                g_OriginalAutoRotationEnabled = g_AutoRotateContext.AutoRotationEnabled;
                LeaveCriticalSection(&g_AutoRotateContext.CriticalSection);

                g_HaveOriginalAutoRotationState = TRUE;
            }

            DisableAutoRotation();

            g_DisabledAutoRotation = TRUE;

            wprintf(
                L"AUTO-ROTATION: disabled "
                L"(mixed-orientation mode)\n");
        }
    }
    else
    {
        if (g_DisabledAutoRotation)
        {
            if (g_OriginalAutoRotationEnabled)
            {
                EnableAutoRotation();
            }
            else
            {
                DisableAutoRotation();
            }

            g_DisabledAutoRotation = FALSE;
            g_HaveOriginalAutoRotationState = FALSE;

            wprintf(L"AUTO-ROTATION: restored\n");
        }
    }

    LeaveCriticalSection(&g_AutoRotationPolicyLock);
}
