#include "pch.h"

#include "Transition\SurfaceFactory.h"

#include <d3d11.h>
#include <dxgi.h>

using Microsoft::WRL::ComPtr;

namespace Transition {

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
        wprintf(L"[Transition][ERROR] Panel texture is null\n");

        return E_INVALIDARG;
    }

    {
        wchar_t buffer[512];

        swprintf_s(
            buffer,
            L"[Transition][INFO] Panel DesktopRect=(%ld,%ld)-(%ld,%ld)\n",
            Panel.DesktopRect.left,
            Panel.DesktopRect.top,
            Panel.DesktopRect.right,
            Panel.DesktopRect.bottom);

        wprintf(buffer);
    }

    Result.DesktopRect = Panel.DesktopRect;

    D3D11_TEXTURE2D_DESC desc{};

    Panel.Texture->GetDesc(&desc);

    Result.Width = desc.Width;

    Result.Height = desc.Height;

    {
        wchar_t buffer[256];

        swprintf_s(buffer, L"[Transition][INFO] Panel Size=%ux%u\n", Result.Width, Result.Height);

        wprintf(buffer);
    }

    RETURN_IF_FAILED(CreateSurfaceFromTexture(Host, Panel.Texture.Get(), Result.Surface.GetAddressOf()));

    wprintf(L"[Transition][INFO] CreateSurfaceFromTexture OK\n");

    RETURN_IF_FAILED(Host.CreateVisual(Result.Visual.GetAddressOf()));

    wprintf(L"[Transition][INFO] CreateVisual OK\n");

    auto *dcomp = Host.GetCompositionDevice();

    if (!dcomp)
    {
        wprintf(L"[Transition][ERROR] CompositionDevice is null\n");

        return E_UNEXPECTED;
    }

    RETURN_IF_FAILED(dcomp->CreateTranslateTransform(Result.Translate.GetAddressOf()));

    wprintf(L"[Transition][INFO] CreateTranslateTransform OK\n");

    RETURN_IF_FAILED(dcomp->CreateScaleTransform(Result.Scale.GetAddressOf()));

    wprintf(L"[Transition][INFO] CreateScaleTransform OK\n");

    RETURN_IF_FAILED(dcomp->CreateRotateTransform(Result.Rotate.GetAddressOf()));

    wprintf(L"[Transition][INFO] CreateRotateTransform OK\n");

    RETURN_IF_FAILED(Result.Translate->SetOffsetX(static_cast<float>(Panel.DesktopRect.left)));

    RETURN_IF_FAILED(Result.Translate->SetOffsetY(static_cast<float>(Panel.DesktopRect.top)));

    wprintf(L"[Transition][INFO] Translate set\n");

    RETURN_IF_FAILED(Result.Scale->SetScaleX(1.0f));

    RETURN_IF_FAILED(Result.Scale->SetScaleY(1.0f));

    wprintf(L"[Transition][INFO] Scale set\n");

    RETURN_IF_FAILED(Result.Rotate->SetAngle(0.0f));

    wprintf(L"[Transition][INFO] Rotation set\n");

    IDCompositionTransform *transforms[]{Result.Scale.Get(), Result.Rotate.Get(), Result.Translate.Get()};

#if !TRANSITION_DISABLE_TRANSFORMGROUPS

    RETURN_IF_FAILED(
        dcomp->CreateTransformGroup(transforms, ARRAYSIZE(transforms), Result.TransformGroup.GetAddressOf()));

    wprintf(L"[Transition][INFO] CreateTransformGroup OK\n");

    RETURN_IF_FAILED(Result.Visual->SetTransform(Result.TransformGroup.Get()));

    wprintf(L"[Transition][INFO] SetTransform(Group) OK\n");

#else

    RETURN_IF_FAILED(Result.Visual->SetTransform(Result.Translate.Get()));

    wprintf(L"[Transition][INFO] SetTransform(Translate) OK\n");

#endif

    RETURN_IF_FAILED(Result.Visual->SetContent(Result.Surface.Get()));

    wprintf(L"[Transition][INFO] SetContent OK\n");

    RETURN_IF_FAILED(Host.AddVisual(Result.Visual.Get(), false));

    wprintf(L"[Transition][INFO] AddVisual OK\n");

    return S_OK;
}

} // namespace Transition