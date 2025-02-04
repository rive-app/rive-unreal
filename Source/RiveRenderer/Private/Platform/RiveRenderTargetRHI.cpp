#include "RiveRenderTargetRHI.h"
#include "RiveRendererRHI.h"
#include "Engine/Texture2DDynamic.h"
THIRD_PARTY_INCLUDES_START
#include "rive/renderer/render_target.hpp"
THIRD_PARTY_INCLUDES_END

FRiveRenderTargetRHI::FRiveRenderTargetRHI(
    const TSharedRef<FRiveRendererRHI>& InRiveRenderer,
    const FName& InRiveName,
    UTexture2DDynamic* InRenderTarget) :
    FRiveRenderTarget(InRiveRenderer, InRiveName, InRenderTarget),
    RiveRenderer(InRiveRenderer)
{}

FRiveRenderTargetRHI::~FRiveRenderTargetRHI() {}

DECLARE_GPU_STAT_NAMED(
    CacheTextureTargetRHI,
    TEXT("FRiveRenderTargetD3D11::CacheTextureTarget_RenderThread"));
void FRiveRenderTargetRHI::CacheTextureTarget_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    const FTextureRHIRef& InTexture)
{
    check(IsInRenderingThread());
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

#if WITH_RIVE
    rive::gpu::RenderContext* PLSRenderContext =
        RiveRenderer->GetRenderContext();
    if (PLSRenderContext == nullptr)
    {
        return;
    }
#endif // WITH_RIVE

    if (!InTexture.IsValid())
    {
        return;
    }

    EPixelFormat PixelFormat = InTexture->GetFormat();

    if (PixelFormat != PF_R8G8B8A8)
    {
        return;
    }

    SCOPED_GPU_STAT(RHICmdList, CacheTextureTargetRHI);
#if WITH_RIVE

    if (CachedRenderTarget)
    {
        CachedRenderTarget.reset();
    }

    RenderContextRHIImpl* const PLSRenderContextImpl =
        PLSRenderContext->static_impl_cast<RenderContextRHIImpl>();
    CachedRenderTarget =
        PLSRenderContextImpl->makeRenderTarget(RHICmdList, InTexture);

#endif
}

void FRiveRenderTargetRHI::Render_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    const TArray<FRiveRenderCommand>& RiveRenderCommands)
{
    FRiveRenderTarget::Render_Internal(RiveRenderCommands);
}

rive::rcp<rive::gpu::RenderTarget> FRiveRenderTargetRHI::GetRenderTarget() const
{
    return CachedRenderTarget;
}