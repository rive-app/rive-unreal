// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererD3D11.h"

#if PLATFORM_WINDOWS
#include "ID3D11DynamicRHI.h"
#include "Logs/RiveRendererLog.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#include "RiveRenderTargetD3D11.h"
#include "Windows/D3D11ThirdParty.h"
#include "D3D11RHIPrivate.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/renderer/d3d/render_context_d3d_impl.hpp"
#include "rive/renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

// ------------- FRiveRendererD3D11GPUAdapter -------------
#if WITH_RIVE
void FRiveRendererD3D11GPUAdapter::Initialize(
    rive::gpu::RenderContextD3DImpl::ContextOptions& OutContextOptions)
{
    check(IsInRenderingThread());

    D3D11RHI = GetID3D11DynamicRHI();
    D3D11DevicePtr = D3D11RHI->RHIGetDevice();
    D3D11DeviceContext = nullptr;
    D3D11DevicePtr->GetImmediateContext(&D3D11DeviceContext);

    if (SUCCEEDED(D3D11DevicePtr->QueryInterface(
            __uuidof(IDXGIDevice),
            (void**)DXGIDevice.GetInitReference())))
    {
        TRefCountPtr<IDXGIAdapter> DXGIAdapter;
        if (SUCCEEDED(DXGIDevice->GetAdapter(DXGIAdapter.GetInitReference())))
        {
            DXGI_ADAPTER_DESC AdapterDesc;
            DXGIAdapter->GetDesc(&AdapterDesc);

            OutContextOptions.isIntel = AdapterDesc.VendorId == 0x163C ||
                                        AdapterDesc.VendorId == 0x8086 ||
                                        AdapterDesc.VendorId == 0x8087;
        }
    }
}
#endif // WITH_RIVE

void FRiveRendererD3D11GPUAdapter::ResetDXState()
{
    // Clear the internal state kept by UE and reset DX
    FD3D11DynamicRHI* DynRHI = static_cast<FD3D11DynamicRHI*>(D3D11RHI);
    if (ensure(DynRHI))
    {
        DynRHI->ClearState();
    }
}

// ------------- FRiveRendererD3D11 -------------

TSharedPtr<IRiveRenderTarget> FRiveRendererD3D11::
    CreateTextureTarget_GameThread(const FName& InRiveName,
                                   UTexture2DDynamic* InRenderTarget)
{
    check(IsInGameThread());

    FScopeLock Lock(&ThreadDataCS);

    const TSharedPtr<FRiveRenderTargetD3D11> RiveRenderTarget =
        MakeShared<FRiveRenderTargetD3D11>(SharedThis(this),
                                           InRiveName,
                                           InRenderTarget);

    RenderTargets.Add(InRiveName, RiveRenderTarget);

    return RiveRenderTarget;
}

DECLARE_GPU_STAT_NAMED(CreateRenderContext,
                       TEXT("CreateRenderContext_RenderThread"));
void FRiveRendererD3D11::CreateRenderContext_RenderThread(
    FRHICommandListImmediate& RHICmdList)
{
    check(IsInRenderingThread());

    FScopeLock Lock(&ThreadDataCS);

    SCOPED_GPU_STAT(RHICmdList, CreateRenderContext);

    if (IsRHID3D11())
    {
#if WITH_RIVE
        rive::gpu::RenderContextD3DImpl::ContextOptions ContextOptions;

        D3D11GPUAdapter = MakeUnique<FRiveRendererD3D11GPUAdapter>();
        D3D11GPUAdapter->Initialize(ContextOptions);

        RenderContext = rive::gpu::RenderContextD3DImpl::MakeContext(
            D3D11GPUAdapter->GetD3D11DevicePtr(),
            D3D11GPUAdapter->GetD3D11DeviceContext(),
            ContextOptions);
#endif // WITH_RIVE
    }
}

void FRiveRendererD3D11::ResetDXState() const
{
    check(IsInRenderingThread());
    check(D3D11GPUAdapter.IsValid());

    FScopeLock Lock(&ThreadDataCS);
    D3D11GPUAdapter->ResetDXState();
}

#endif // PLATFORM_WINDOWS
