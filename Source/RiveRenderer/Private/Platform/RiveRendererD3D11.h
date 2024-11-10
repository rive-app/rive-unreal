// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RiveRenderer.h"

#if PLATFORM_WINDOWS

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinWindows.h"
#include "rive/renderer/d3d/render_context_d3d_impl.hpp" // This ends up including windows.h
#include "Windows/HideWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_END

struct ID3D11DynamicRHI;

namespace rive::gpu
{
class RenderTargetD3D;
class RenderContextD3DImpl;
} // namespace rive::gpu

#endif // WITH_RIVE

class FRiveRendererD3D11GPUAdapter
{
public:
    FRiveRendererD3D11GPUAdapter() {}

#if WITH_RIVE
    void Initialize(
        rive::gpu::RenderContextD3DImpl::ContextOptions& OutContextOptions);
#endif // WITH_RIVE
    void ResetDXState();

    ID3D11DynamicRHI* GetD3D11RHI() const { return D3D11RHI; }
    ID3D11Device* GetD3D11DevicePtr() const { return D3D11DevicePtr; }
    ID3D11DeviceContext* GetD3D11DeviceContext() const
    {
        return D3D11DeviceContext;
    }
    IDXGIDevice* GetDXGIDevice() const { return DXGIDevice; }

private:
    ID3D11DynamicRHI* D3D11RHI = nullptr;
    ID3D11Device* D3D11DevicePtr = nullptr;
    ID3D11DeviceContext* D3D11DeviceContext = nullptr;
    TRefCountPtr<IDXGIDevice> DXGIDevice = nullptr;
};

class RIVERENDERER_API FRiveRendererD3D11 : public FRiveRenderer
{
    /**
     * Structor(s)
     */
public:
    //~ BEGIN : IRiveRenderer Interface
    virtual TSharedPtr<IRiveRenderTarget> CreateTextureTarget_GameThread(
        const FName& InRiveName,
        UTexture2DDynamic* InRenderTarget) override;
    virtual void CreateRenderContext_RenderThread(
        FRHICommandListImmediate& RHICmdList) override;
    //~ END : IRiveRenderer Interface

    void ResetDXState() const;

private:
    TUniquePtr<FRiveRendererD3D11GPUAdapter> D3D11GPUAdapter;
};

#endif // PLATFORM_WINDOWS
