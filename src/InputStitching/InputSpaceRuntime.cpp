#include "pch.h"

#include "InputStitching\InputSpaceRuntime.h"

#include <windows.h>

#include "InputStitching\InputSpaceConfiguration.h"
#include "InputStitching\InputSpaceGlobals.h"
#include "InputStitching\InputSpaceManager.h"
#include <OrientationManager.h>

namespace InputStitching {

InputSpaceRuntime g_InputSpaceRuntime;

HRESULT
InputSpaceRuntime::Initialize()
{
    if (m_Initialized)
    {
        return S_OK;
    }

    HRESULT hr = LoadInputSpaceConfiguration(&g_InputSpaceConfiguration);

    if (FAILED(hr))
    {
        return hr;
    }

    m_CurrentPolicy = {};

    m_Initialized = true;

    return S_OK;
}

HRESULT
InputSpaceRuntime::Start()
{
    HRESULT hr = ERROR_SUCCESS;

    if (!m_Initialized)
    {
        hr = Initialize();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (m_Running)
    {
        return S_OK;
    }

    hr = Rebuild();
    if (FAILED(hr))
    {
        return hr;
    }

    m_Running = true;

    return S_OK;
}

void
InputSpaceRuntime::Stop()
{
    if (m_Published)
    {
        g_InputSpaceManager.Remove();

        m_Published = false;
    }

    m_Running = false;
}

bool
InputSpaceRuntime::IsRunning() const
{
    return m_Running;
}

HRESULT
InputSpaceRuntime::Rebuild()
{
    ++m_LastGeneration;

    return ApplyPolicy(m_CurrentPolicy);
}

HRESULT
InputSpaceRuntime::GeneratePolicy(const DisplayPolicy &Display, InputSpacePolicy *Policy)
{
    if (!Policy)
    {
        return E_POINTER;
    }

    *Policy = {};

    bool singleScreen = (Display.Display1Enabled != Display.Display2Enabled);

    if (singleScreen)
    {
        if (Display.Display1Enabled)
        {
            Policy->Mode = InputTopologyMode::SingleLeft;
        }
        else
        {
            Policy->Mode = InputTopologyMode::SingleRight;
        }
    }
    else
    {
        bool stitched = g_InputSpaceConfiguration.EnableStitching;

        Policy->Mode = stitched ? InputTopologyMode::DualStitched : InputTopologyMode::DualIndependent;
    }

    Policy->EnableGapRegion = g_InputSpaceConfiguration.EnableGapRegion;

    Policy->EnableStitching = g_InputSpaceConfiguration.EnableStitching;

    Policy->GapPixels = g_InputSpaceConfiguration.GapPixels;

    Policy->PublishToSystem = g_InputSpaceConfiguration.AutoPublish;

    return S_OK;
}

HRESULT
InputSpaceRuntime::ApplyPolicy(const InputSpacePolicy &Policy)
{
    if (g_InputSpaceConfiguration.AutoPublish && m_Published)
    {
        HRESULT hr = g_InputSpaceManager.Remove();

        if (FAILED(hr))
        {
            return hr;
        }

        m_Published = false;
    }

    m_CurrentPolicy = Policy;

    HRESULT hr = g_InputSpaceManager.ApplyPolicy(Policy);

    if (FAILED(hr))
    {
        return hr;
    }

    if (g_InputSpaceConfiguration.AutoPublish)
    {
        hr = g_InputSpaceManager.Publish();

        if (FAILED(hr))
        {
            return hr;
        }

        m_Published = true;
    }

    return S_OK;
}

HRESULT
InputSpaceRuntime::NotifyDisplayTopologyChanged()
{
    if (!m_Running)
    {
        return S_FALSE;
    }

    if (!g_InputSpaceConfiguration.AutoRebuild)
    {
        return S_FALSE;
    }

    return Rebuild();
}

HRESULT
InputSpaceRuntime::NotifyPostureChanged()
{
    if (!m_Running)
    {
        return S_FALSE;
    }

    if (!g_InputSpaceConfiguration.AutoRebuild)
    {
        return S_FALSE;
    }

    return Rebuild();
}

} // namespace InputStitching