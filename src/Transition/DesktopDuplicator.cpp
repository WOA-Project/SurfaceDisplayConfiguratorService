#include "pch.h"

#include "Transition\DesktopDuplicator.h"

#include "Transition\TransitionStatistics.h"
#include "Transition\TransitionTrace.h"

#include <dxgi.h>

using Microsoft::WRL::ComPtr;

namespace Transition {

HRESULT
DesktopDuplicator::Initialize(ID3D11Device *Device, UINT OutputIndex)
{
    if (!Device)
    {
        return E_INVALIDARG;
    }

    m_Device = Device;
    m_OutputIndex = OutputIndex;

    RETURN_IF_FAILED(CreateDuplication());

    m_State = DuplicationState::Active;

    m_GenerationTracker.Synchronize();

    return S_OK;
}

HRESULT
DesktopDuplicator::Recreate(ID3D11Device *Device, UINT OutputIndex)
{
    std::scoped_lock lock(m_RebuildLock);

    m_State = DuplicationState::Reinitializing;

    Shutdown();

    m_Device = Device;
    m_OutputIndex = OutputIndex;

    RETURN_IF_FAILED(CreateDuplication());

    m_State = DuplicationState::Active;

    m_GenerationTracker.Synchronize();

    return S_OK;
}

void
DesktopDuplicator::Shutdown()
{
    m_Duplication.Reset();

    m_DesktopRect = {};

    m_State = DuplicationState::Uninitialized;
}

HRESULT
DesktopDuplicator::CreateDuplication()
{
    ComPtr<IDXGIDevice> dxgiDevice;

    RETURN_IF_FAILED(m_Device.As(&dxgiDevice));

    ComPtr<IDXGIAdapter> adapter;

    RETURN_IF_FAILED(dxgiDevice->GetAdapter(&adapter));

    ComPtr<IDXGIOutput> output;

    RETURN_IF_FAILED(adapter->EnumOutputs(m_OutputIndex, &output));

    DXGI_OUTPUT_DESC desc{};

    output->GetDesc(&desc);

    m_DesktopRect = desc.DesktopCoordinates;

    ComPtr<IDXGIOutput1> output1;

    RETURN_IF_FAILED(output.As(&output1));

    RETURN_IF_FAILED(output1->DuplicateOutput(m_Device.Get(), &m_Duplication));

    return S_OK;
}

HRESULT
DesktopDuplicator::EnsureValid()
{
    if (m_GenerationTracker.IsStale())
    {
        RETURN_IF_FAILED(Recreate(m_Device.Get(), m_OutputIndex));

        m_GenerationTracker.Synchronize();
    }

    switch (m_State)
    {
    case DuplicationState::Active:

        return S_OK;

    case DuplicationState::AccessLost:

    case DuplicationState::DeviceRemoved:

        return Recreate(m_Device.Get(), m_OutputIndex);

    default:

        return E_FAIL;
    }
}

HRESULT
DesktopDuplicator::CaptureSnapshot(ComPtr<ID3D11Texture2D> &Snapshot)
{
    Snapshot.Reset();

    RETURN_IF_FAILED(EnsureValid());

    DXGI_OUTDUPL_FRAME_INFO
    frameInfo{};

    ComPtr<IDXGIResource> resource;

    const HRESULT hr = m_Duplication->AcquireNextFrame(100, &frameInfo, &resource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
    {
        return S_FALSE;
    }

    if (hr == DXGI_ERROR_ACCESS_LOST)
    {
        m_State = DuplicationState::AccessLost;

        TransitionStatistics::RecordAccessLost();

        TransitionTrace::Write(TraceLevel::Warning, TraceEvent::AccessLost, L"Desktop duplication access lost");

        return HRESULT_FROM_WIN32(ERROR_RETRY);
    }

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        m_State = DuplicationState::DeviceRemoved;

        TransitionStatistics::RecordDeviceRemoved();

        TransitionTrace::Write(TraceLevel::Warning, TraceEvent::DeviceRemoved, L"Desktop duplication device removed");

        return HRESULT_FROM_WIN32(ERROR_RETRY);
    }

    RETURN_IF_FAILED(hr);

    struct FrameRelease
    {
        IDXGIOutputDuplication *Duplication;

        ~FrameRelease()
        {
            if (Duplication)
            {
                Duplication->ReleaseFrame();
            }
        }
    } releaseFrame{m_Duplication.Get()};

    ComPtr<ID3D11Texture2D> frameTexture;

    RETURN_IF_FAILED(resource.As(&frameTexture));

    D3D11_TEXTURE2D_DESC
    textureDesc{};

    frameTexture->GetDesc(&textureDesc);

    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    textureDesc.Usage = D3D11_USAGE_DEFAULT;

    textureDesc.CPUAccessFlags = 0;

    textureDesc.MiscFlags = 0;

    RETURN_IF_FAILED(m_Device->CreateTexture2D(&textureDesc, nullptr, Snapshot.GetAddressOf()));

    ComPtr<ID3D11DeviceContext> context;

    m_Device->GetImmediateContext(&context);

    context->CopyResource(Snapshot.Get(), frameTexture.Get());

    context->Flush();

    return S_OK;
}

} // namespace Transition
