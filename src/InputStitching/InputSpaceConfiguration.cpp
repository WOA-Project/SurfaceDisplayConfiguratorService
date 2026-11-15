#include "pch.h"

#include "InputStitching\InputSpaceConfiguration.h"

namespace InputStitching {

InputSpaceConfiguration g_InputSpaceConfiguration;

void
ResetInputSpaceConfiguration(InputSpaceConfiguration *Configuration)
{
    if (!Configuration)
    {
        return;
    }

    Configuration->AutoPublish = true;

    Configuration->AutoRebuild = true;

    Configuration->EnableStitching = true;

    Configuration->EnableGapRegion = true;

    Configuration->GapPixels = 84;

    Configuration->FallbackMode = InputTopologyMode::DualStitched;
}

HRESULT
LoadInputSpaceConfiguration(InputSpaceConfiguration *Configuration)
{
    if (!Configuration)
    {
        return E_POINTER;
    }

    //
    // For now we simply use defaults.
    //
    ResetInputSpaceConfiguration(Configuration);

    return S_OK;
}

HRESULT
SaveInputSpaceConfiguration(const InputSpaceConfiguration *Configuration)
{
    if (!Configuration)
    {
        return E_POINTER;
    }

    //
    // Registry persistence will be
    // implemented later.
    //
    UNREFERENCED_PARAMETER(Configuration);

    return S_OK;
}

} // namespace InputStitching