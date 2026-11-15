#pragma once

#include <windows.h>

#include "InputSpacePolicy.h"
#include <OrientationManager.h>

namespace InputStitching {

class InputSpaceRuntime
{
public:
    InputSpaceRuntime() = default;

    HRESULT Initialize();

    HRESULT Start();

    void Stop();

    HRESULT Rebuild();

    bool IsRunning() const;

    HRESULT ApplyPolicy(const InputSpacePolicy &Policy);

private:
    HRESULT GeneratePolicy(const DisplayPolicy &DisplayPolicy, InputSpacePolicy *Policy);

    HRESULT NotifyDisplayTopologyChanged();

    HRESULT NotifyPostureChanged();

private:
    ULONG m_LastGeneration = 0;

    bool m_Published = false;

    bool m_Initialized = false;

    bool m_Running = false;

    InputSpacePolicy m_CurrentPolicy{};
};

extern InputSpaceRuntime g_InputSpaceRuntime;

} // namespace InputStitching