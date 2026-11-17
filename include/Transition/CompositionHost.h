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
    HRESULT Initialize(HWND Window);

    void Shutdown();

    HRESULT Commit();

    HRESULT CreateVisual(IDCompositionVisual **Visual);

    HRESULT CreateSurface(UINT Width, UINT Height, DXGI_FORMAT Format, IDCompositionSurface **Surface);

    HRESULT AddVisual(IDCompositionVisual *Visual, bool CommitImmediately);

    HRESULT RemoveVisual(IDCompositionVisual *Visual, bool CommitImmediately);

    ID3D11Device *GetD3DDevice() const noexcept { return m_D3DDevice.Get(); }

    ID3D11DeviceContext *GetImmediateContext() const noexcept { return m_D3DContext.Get(); }

    IDCompositionDevice *GetCompositionDevice() const noexcept { return m_CompositionDevice.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_D3DDevice;

    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_D3DContext;

    Microsoft::WRL::ComPtr<IDXGIDevice> m_DxgiDevice;

    Microsoft::WRL::ComPtr<IDCompositionDevice> m_CompositionDevice;

    Microsoft::WRL::ComPtr<IDCompositionTarget> m_CompositionTarget;

    Microsoft::WRL::ComPtr<IDCompositionVisual> m_RootVisual;
};

} // namespace Transition