// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RiveRenderTargetRHI.h"

#include "RiveRendererModule.h"
#include "RiveRendererRHI.h"
#include "Engine/Texture2DDynamic.h"

THIRD_PARTY_INCLUDES_START
#include "rive/renderer/render_target.hpp"
THIRD_PARTY_INCLUDES_END

FRiveRenderTargetRHI::FRiveRenderTargetRHI(FRiveRenderer* Renderer,
                                           const FName& InRiveName,
                                           UTexture2DDynamic* InRenderTarget) :
    FRiveRenderTarget(Renderer, InRiveName, InRenderTarget)
{}

FRiveRenderTargetRHI::FRiveRenderTargetRHI(
    FRiveRenderer* Renderer,
    const FName& InRiveName,
    UTextureRenderTarget2D* InRenderTarget) :
    FRiveRenderTarget(Renderer, InRiveName, InRenderTarget)
{}

FRiveRenderTargetRHI::FRiveRenderTargetRHI(FRiveRenderer* Renderer,
                                           const FName& InRiveName,
                                           FRenderTarget* InRenderTarget) :
    FRiveRenderTarget(Renderer, InRiveName, InRenderTarget)
{
    ENQUEUE_RENDER_COMMAND(FRiveRenderTargetRHI_CacheRenderTarget)
    ([this](FRHICommandListImmediate& RHICmdList) {
        rive::gpu::RenderContext* PLSRenderContext =
            RiveRenderer->GetRenderContext();
        check(PLSRenderContext);
        RenderContextRHIImpl* const PLSRenderContextImpl =
            PLSRenderContext->static_impl_cast<RenderContextRHIImpl>();
        CachedRenderTarget =
            PLSRenderContextImpl->makeRenderTarget(RHICmdList,
                                                   ThumbnailRenderTarget);
    });
}

FRiveRenderTargetRHI::FRiveRenderTargetRHI(FRDGBuilder& GraphBuilder,
                                           FRiveRenderer* Renderer,
                                           const FName& InRiveName,
                                           FRDGTextureRef InRenderTarget) :
    FRiveRenderTarget(Renderer, InRiveName, InRenderTarget)
{
    check(IsInRenderingThread());
    check(RiveRenderer != nullptr);

    rive::gpu::RenderContext* PLSRenderContext =
        RiveRenderer->GetRenderContext();
    RenderContextRHIImpl* const PLSRenderContextImpl =
        PLSRenderContext->static_impl_cast<RenderContextRHIImpl>();
    CachedRenderTarget =
        PLSRenderContextImpl->makeRenderTarget(GraphBuilder, InRenderTarget);
}

FRiveRenderTargetRHI::~FRiveRenderTargetRHI() {}

DECLARE_GPU_STAT_NAMED(
    CacheTextureTargetRHI,
    TEXT("FRiveRenderTargetD3D11::CacheTextureTarget_RenderThread"));
void FRiveRenderTargetRHI::CacheTextureTarget_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    const FTextureRHIRef& InTexture)
{
    check(IsInRenderingThread());
    check(RiveRenderer != nullptr);

    rive::gpu::RenderContext* PLSRenderContext =
        RiveRenderer->GetRenderContext();

    check(PLSRenderContext);

    if (!InTexture.IsValid())
    {
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("FRiveRenderTargetRHI::CacheTextureTarget_RenderThread "
                    "InTexture was invalid !"));
        return;
    }

    EPixelFormat PixelFormat = InTexture->GetFormat();

    if (PixelFormat != PF_R8G8B8A8)
    {
        UE_LOG(LogRiveRenderer,
               Warning,
               TEXT("FRiveRenderTargetRHI::CacheTextureTarget_RenderThread "
                    "InTexture was not PF_R8G8B8A8!"));
    }

    SCOPED_GPU_STAT(RHICmdList, CacheTextureTargetRHI);

    RenderContextRHIImpl* const PLSRenderContextImpl =
        PLSRenderContext->static_impl_cast<RenderContextRHIImpl>();
    CachedRenderTarget =
        PLSRenderContextImpl->makeRenderTarget(RHICmdList, InTexture);
}

void FRiveRenderTargetRHI::UpdateTargetTexture(FRDGTextureRef InRenderTarget)
{
    FRiveRenderTarget::UpdateTargetTexture(InRenderTarget);
    CachedRenderTarget->updateTargetTexture(InRenderTarget);
}

void FRiveRenderTargetRHI::UpdateTargetTexture(FRenderTarget* InRenderTarget)
{
    check(IsInGameThread());
    FRiveRenderTarget::UpdateTargetTexture(InRenderTarget);
    ENQUEUE_RENDER_COMMAND(FRiveRenderTargetRHI_CacheRenderTarget)
    ([this, InRenderTarget](FRHICommandListImmediate& RHICmdList) {
        CachedRenderTarget->updateTargetTexture(InRenderTarget);
    });
}

void FRiveRenderTargetRHI::UpdateRHIResorourceDirect(
    FTextureRHIRef InRenderTarget)
{
    FRiveRenderTarget::UpdateRHIResorourceDirect(InRenderTarget);
    CacheTextureTarget_RenderThread(GRHICommandList.GetImmediateCommandList(),
                                    InRenderTarget);
}