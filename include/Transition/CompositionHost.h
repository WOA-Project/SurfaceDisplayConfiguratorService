#pragma once

#include <windows.h>

#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_2.h>

#include <wrl/client.h>

namespace Transition {

class CompositionHost
{
public:
    CompositionHost() = default;
    ~CompositionHost() = default;

    CompositionHost(const CompositionHost &) = delete;

    CompositionHost &operator=(const CompositionHost &) = delete;

    HRESULT Initialize(HWND Window);

    void Shutdown();

    HRESULT Commit();

    HRESULT CreateVisual(IDCompositionVisual **Visual);

    HRESULT CreateSurface(UINT Width, UINT Height, DXGI_FORMAT Format, IDCompositionSurface **Surface);

    HRESULT AddVisual(IDCompositionVisual *Visual, bool CommitImmediately = false);

    HRESULT RemoveVisual(IDCompositionVisual *Visual, bool CommitImmediately = false);

    ID3D11Device *GetD3DDevice() const noexcept { return m_D3DDevice.Get(); }

    ID3D11DeviceContext *GetImmediateContext() const noexcept { return m_D3DContext.Get(); }

    IDXGIDevice *GetDxgiDevice() const noexcept { return m_DxgiDevice.Get(); }

    IDCompositionDevice *GetCompositionDevice() const noexcept { return m_CompositionDevice.Get(); }

    IDCompositionTarget *GetCompositionTarget() const noexcept { return m_CompositionTarget.Get(); }

    IDCompositionVisual *GetRootVisual() const noexcept { return m_RootVisual.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_D3DDevice;

    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_D3DContext;

    Microsoft::WRL::ComPtr<IDXGIDevice> m_DxgiDevice;

    Microsoft::WRL::ComPtr<IDCompositionDevice> m_CompositionDevice;

    Microsoft::WRL::ComPtr<IDCompositionTarget> m_CompositionTarget;

    Microsoft::WRL::ComPtr<IDCompositionVisual> m_RootVisual;
};

} // namespace Transition