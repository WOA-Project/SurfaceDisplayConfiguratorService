#include "pch.h"

#include "Transition\DesktopCaptureManager.h"

#include "Transition\DisplayTopologyFingerprint.h"
#include "Transition\TransitionStatistics.h"
#include "Transition\TransitionTrace.h"

#include <dxgi.h>

using Microsoft::WRL::ComPtr;

namespace Transition {

HRESULT
DesktopCaptureManager::Initialize(ID3D11Device *Device)
{
    if (!Device)
    {
        return E_INVALIDARG;
    }

    m_Device = Device;

    RETURN_IF_FAILED(EnumerateOutputs());

    DisplayTopologyCapture::Capture(m_LastTopology);

    return S_OK;
}

void
DesktopCaptureManager::Shutdown()
{
    for (auto &duplicator : m_Duplicators)
    {
        duplicator->Shutdown();
    }

    m_Duplicators.clear();

    m_Device.Reset();

    m_State = CaptureState::Suspended;
}

HRESULT
DesktopCaptureManager::EnumerateOutputs()
{
    m_Duplicators.clear();

    ComPtr<IDXGIDevice> dxgiDevice;

    RETURN_IF_FAILED(m_Device.As(&dxgiDevice));

    ComPtr<IDXGIAdapter> adapter;

    RETURN_IF_FAILED(dxgiDevice->GetAdapter(&adapter));

    UINT outputIndex = 0;

    while (true)
    {
        ComPtr<IDXGIOutput> output;

        const HRESULT hr = adapter->EnumOutputs(outputIndex, &output);

        if (hr == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }

        RETURN_IF_FAILED(hr);

        auto duplicator = std::make_unique<DesktopDuplicator>();

        RETURN_IF_FAILED(duplicator->Initialize(m_Device.Get(), outputIndex));

        m_Duplicators.emplace_back(std::move(duplicator));

        ++outputIndex;
    }

    return S_OK;
}

HRESULT
DesktopCaptureManager::Capture(CapturedDesktop &Desktop)
{
    if (m_State != CaptureState::Active)
    {
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    ScopedTrace trace(TraceEvent::CaptureBegin, TraceEvent::CaptureEnd, L"DesktopCaptureManager::Capture");

    Desktop.Clear();

    DisplayTopologyFingerprint currentTopology;

    if (SUCCEEDED(DisplayTopologyCapture::Capture(currentTopology)))
    {
        if (currentTopology != m_LastTopology)
        {
            TransitionStatistics::RecordTopologyChange();

            RETURN_IF_FAILED(Rebuild());

            m_LastTopology = currentTopology;
        }
    }

    for (UINT i = 0; i < m_Duplicators.size(); ++i)
    {
        ComPtr<ID3D11Texture2D> snapshot;

        const HRESULT hr = m_Duplicators[i]->CaptureSnapshot(snapshot);

        if (hr == S_FALSE)
        {
            continue;
        }

        if (hr == HRESULT_FROM_WIN32(ERROR_RETRY))
        {
            continue;
        }

        RETURN_IF_FAILED(hr);

        CapturedPanel panel;

        panel.OutputIndex = i;

        panel.DesktopRect = m_Duplicators[i]->GetDesktopRect();

        panel.Texture = std::move(snapshot);

        Desktop.Panels.emplace_back(std::move(panel));
    }

    return S_OK;
}

HRESULT
DesktopCaptureManager::Rebuild()
{
    ScopedTrace trace(TraceEvent::RebuildBegin, TraceEvent::RebuildEnd, L"DesktopCaptureManager::Rebuild");

    m_State = CaptureState::Rebuilding;

    for (auto &duplicator : m_Duplicators)
    {
        duplicator->Shutdown();
    }

    m_Duplicators.clear();

    RETURN_IF_FAILED(EnumerateOutputs());

    DisplayTopologyCapture::Capture(m_LastTopology);

    m_State = CaptureState::Active;

    return S_OK;
}

void
DesktopCaptureManager::Pause()
{
    m_State = CaptureState::TopologyChanging;
}

void
DesktopCaptureManager::Resume()
{
    m_State = CaptureState::Active;
}

} // namespace Transition
