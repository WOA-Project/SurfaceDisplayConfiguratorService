#include "pch.h"

#include "Transition\CompositionHost.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "dxgi.lib")

namespace Transition {

HRESULT
CompositionHost::Initialize(HWND Window)
{
    UINT Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
    Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    RETURN_IF_FAILED(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        Flags,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        m_D3DDevice.GetAddressOf(),
        nullptr,
        m_D3DContext.GetAddressOf()));

    RETURN_IF_FAILED(m_D3DDevice.As(&m_DxgiDevice));

    RETURN_IF_FAILED(DCompositionCreateDevice(
        m_DxgiDevice.Get(),
        __uuidof(IDCompositionDevice),
        reinterpret_cast<void **>(m_CompositionDevice.GetAddressOf())));

    RETURN_IF_FAILED(m_CompositionDevice->CreateTargetForHwnd(Window, TRUE, m_CompositionTarget.GetAddressOf()));

    RETURN_IF_FAILED(m_CompositionDevice->CreateVisual(m_RootVisual.GetAddressOf()));

    RETURN_IF_FAILED(m_CompositionTarget->SetRoot(m_RootVisual.Get()));

    return Commit();
}

void
CompositionHost::Shutdown()
{
    m_RootVisual.Reset();

    m_CompositionTarget.Reset();

    m_CompositionDevice.Reset();

    m_DxgiDevice.Reset();

    m_D3DContext.Reset();

    m_D3DDevice.Reset();
}

HRESULT
CompositionHost::Commit()
{
    if (!m_CompositionDevice)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = m_CompositionDevice->Commit();

    wchar_t buffer[128];

    swprintf_s(buffer, L"[Transition][INFO] Composition Commit hr=0x%08X\n", hr);

    wprintf(buffer);

    return hr;
}

HRESULT
CompositionHost::CreateVisual(IDCompositionVisual **Visual)
{
    if (!Visual)
    {
        return E_INVALIDARG;
    }

    return m_CompositionDevice->CreateVisual(Visual);
}

HRESULT
CompositionHost::CreateSurface(UINT Width, UINT Height, DXGI_FORMAT Format, IDCompositionSurface **Surface)
{
    if (!Surface)
    {
        return E_INVALIDARG;
    }

    return m_CompositionDevice->CreateSurface(Width, Height, Format, DXGI_ALPHA_MODE_IGNORE, Surface);
}

HRESULT
CompositionHost::AddVisual(IDCompositionVisual *Visual, bool CommitImmediately)
{
    if (!Visual)
    {
        return E_INVALIDARG;
    }

    RETURN_IF_FAILED(m_RootVisual->AddVisual(Visual, FALSE, nullptr));

    if (CommitImmediately)
    {
        RETURN_IF_FAILED(Commit());
    }

    return S_OK;
}

HRESULT
CompositionHost::RemoveVisual(IDCompositionVisual *Visual, bool CommitImmediately)
{
    if (!Visual)
    {
        return E_INVALIDARG;
    }

    RETURN_IF_FAILED(m_RootVisual->RemoveVisual(Visual));

    if (CommitImmediately)
    {
        RETURN_IF_FAILED(Commit());
    }

    return S_OK;
}

} // namespace Transition
