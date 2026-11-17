#include "pch.h"

#include "Transition\SurfaceFactory.h"

#include <d3d11.h>
#include <dxgi.h>

using Microsoft::WRL::ComPtr;

namespace Transition {

HRESULT
SurfaceFactory::CreateDebugSurface(
    CompositionHost &Host,
    UINT Width,
    UINT Height,
    D2D1_COLOR_F Color,
    IDCompositionSurface **Surface)
{
    *Surface = nullptr;

    RETURN_IF_FAILED(Host.CreateSurface(Width, Height, DXGI_FORMAT_B8G8R8A8_UNORM, Surface));

    POINT offset{};

    Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurface;

    RETURN_IF_FAILED((*Surface)->BeginDraw(nullptr, IID_PPV_ARGS(dxgiSurface.GetAddressOf()), &offset));

    HRESULT hr = S_OK;

    do
    {
        Microsoft::WRL::ComPtr<ID2D1Factory1> factory;

        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf());

        if (FAILED(hr))
        {
            break;
        }

        Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice;

        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;

        hr = Host.GetD3DDevice()->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()));

        if (FAILED(hr))
        {
            break;
        }

        hr = factory->CreateDevice(dxgiDevice.Get(), d2dDevice.GetAddressOf());

        if (FAILED(hr))
        {
            break;
        }

        Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dContext;

        hr = d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2dContext.GetAddressOf());

        if (FAILED(hr))
        {
            break;
        }

        Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;

        D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

        hr = d2dContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &props, bitmap.GetAddressOf());

        if (FAILED(hr))
        {
            break;
        }

        d2dContext->SetTarget(bitmap.Get());

        d2dContext->BeginDraw();

        d2dContext->Clear(Color);

        hr = d2dContext->EndDraw();
    } while (false);

    const HRESULT endDrawHr = (*Surface)->EndDraw();

    if (FAILED(hr))
    {
        return hr;
    }

    return endDrawHr;
}

HRESULT
SurfaceFactory::CreateSurfaceFromTexture(
    CompositionHost &Host,
    ID3D11Texture2D *SourceTexture,
    IDCompositionSurface **Surface)
{
    if (!SourceTexture || !Surface)
    {
        wprintf(L"[Transition][ERROR] CreateSurfaceFromTexture invalid args\n");

        return E_INVALIDARG;
    }

    *Surface = nullptr;

    D3D11_TEXTURE2D_DESC textureDesc{};

    SourceTexture->GetDesc(&textureDesc);

    {
        wchar_t buffer[512];

        swprintf_s(
            buffer,
            L"[Transition][INFO] SourceTexture "
            L"%ux%u Format=%u Usage=%u BindFlags=0x%08X MiscFlags=0x%08X\n",
            textureDesc.Width,
            textureDesc.Height,
            textureDesc.Format,
            textureDesc.Usage,
            textureDesc.BindFlags,
            textureDesc.MiscFlags);

        wprintf(buffer);
    }

    HRESULT hr = Host.CreateSurface(textureDesc.Width, textureDesc.Height, textureDesc.Format, Surface);

    {
        wchar_t buffer[128];

        swprintf_s(buffer, L"[Transition][INFO] CreateSurface hr=0x%08X\n", hr);

        wprintf(buffer);
    }

    RETURN_IF_FAILED(hr);

    POINT updateOffset{};

    ComPtr<IDXGISurface> drawSurface;

    hr = (*Surface)->BeginDraw(nullptr, IID_PPV_ARGS(drawSurface.GetAddressOf()), &updateOffset);

    {
        wchar_t buffer[256];

        swprintf_s(
            buffer, L"[Transition][INFO] BeginDraw hr=0x%08X Offset=(%ld,%ld)\n", hr, updateOffset.x, updateOffset.y);

        wprintf(buffer);
    }

    RETURN_IF_FAILED(hr);

    HRESULT result = S_OK;

    do
    {
        ComPtr<ID3D11Texture2D> destinationTexture;

        result = drawSurface.As(&destinationTexture);

        {
            wchar_t buffer[128];

            swprintf_s(buffer, L"[Transition][INFO] IDXGISurface->ID3D11Texture2D hr=0x%08X\n", result);

            wprintf(buffer);
        }

        if (FAILED(result))
        {
            break;
        }

        D3D11_TEXTURE2D_DESC destinationDesc{};

        destinationTexture->GetDesc(&destinationDesc);

        {
            wchar_t buffer[512];

            swprintf_s(
                buffer,
                L"[Transition][INFO] DestinationTexture "
                L"%ux%u Format=%u Usage=%u BindFlags=0x%08X MiscFlags=0x%08X\n",
                destinationDesc.Width,
                destinationDesc.Height,
                destinationDesc.Format,
                destinationDesc.Usage,
                destinationDesc.BindFlags,
                destinationDesc.MiscFlags);

            wprintf(buffer);
        }

        auto *context = Host.GetImmediateContext();

        if (!context)
        {
            wprintf(L"[Transition][ERROR] ImmediateContext is null\n");

            result = E_UNEXPECTED;
            break;
        }

        wprintf(L"[Transition][INFO] CopyResource begin\n");

        /*context->CopyResource(
            destinationTexture.Get(),
            SourceTexture);*/

        D3D11_BOX sourceBox{};

        sourceBox.left = 0;
        sourceBox.top = 0;
        sourceBox.front = 0;

        sourceBox.right = textureDesc.Width;
        sourceBox.bottom = textureDesc.Height;
        sourceBox.back = 1;

        context->CopySubresourceRegion(destinationTexture.Get(), 0, 0, 0, 0, SourceTexture, 0, &sourceBox);

        context->Flush();

        wprintf(L"[Transition][INFO] CopyResource end\n");
    } while (false);

    const HRESULT endDrawHr = (*Surface)->EndDraw();

    {
        wchar_t buffer[128];

        swprintf_s(buffer, L"[Transition][INFO] EndDraw hr=0x%08X\n", endDrawHr);

        wprintf(buffer);
    }

    if (FAILED(result))
    {
        wprintf(L"[Transition][ERROR] CreateSurfaceFromTexture failed before EndDraw\n");

        return result;
    }

    return endDrawHr;
}

HRESULT
SurfaceFactory::CreatePanelVisual(CompositionHost &Host, const CapturedPanel &Panel, PanelVisual &Result)
{
    Result = {};

    if (!Panel.Texture)
    {
        return E_INVALIDARG;
    }

    Result.DesktopRect = Panel.DesktopRect;

    D3D11_TEXTURE2D_DESC desc{};

    Panel.Texture->GetDesc(&desc);

    Result.Width = desc.Width;

    Result.Height = desc.Height;

    RETURN_IF_FAILED(CreateSurfaceFromTexture(Host, Panel.Texture.Get(), Result.Surface.GetAddressOf()));

    RETURN_IF_FAILED(Host.CreateVisual(Result.Visual.GetAddressOf()));

    auto *dcomp = Host.GetCompositionDevice();

    if (!dcomp)
    {
        return E_UNEXPECTED;
    }

    RETURN_IF_FAILED(dcomp->CreateTranslateTransform(Result.Translate.GetAddressOf()));

    RETURN_IF_FAILED(dcomp->CreateScaleTransform(Result.Scale.GetAddressOf()));

    RETURN_IF_FAILED(dcomp->CreateRotateTransform(Result.Rotate.GetAddressOf()));

    RETURN_IF_FAILED(Result.Translate->SetOffsetX(static_cast<float>(Panel.DesktopRect.left)));

    RETURN_IF_FAILED(Result.Translate->SetOffsetY(static_cast<float>(Panel.DesktopRect.top)));

    RETURN_IF_FAILED(Result.Scale->SetScaleX(1.0f));

    RETURN_IF_FAILED(Result.Scale->SetScaleY(1.0f));

    RETURN_IF_FAILED(Result.Rotate->SetAngle(0.0f));

#if !TRANSITION_DISABLE_TRANSFORMGROUPS

    IDCompositionTransform *transforms[]{Result.Scale.Get(), Result.Rotate.Get(), Result.Translate.Get()};

    RETURN_IF_FAILED(
        dcomp->CreateTransformGroup(transforms, ARRAYSIZE(transforms), Result.TransformGroup.GetAddressOf()));

    RETURN_IF_FAILED(Result.Visual->SetTransform(Result.TransformGroup.Get()));

#else

    RETURN_IF_FAILED(Result.Visual->SetTransform(Result.Translate.Get()));

#endif

    RETURN_IF_FAILED(Result.Visual->SetContent(Result.Surface.Get()));

    RETURN_IF_FAILED(Host.AddVisual(Result.Visual.Get(), false));

    return S_OK;
}

} // namespace Transition