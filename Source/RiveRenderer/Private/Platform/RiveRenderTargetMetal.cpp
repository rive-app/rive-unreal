// Copyright Rive, Inc. All rights reserved.

#include "Platform/RiveRenderTargetMetal.h"

#if PLATFORM_APPLE

#include "DynamicRHI.h"
#include "Engine/Texture2DDynamic.h"
#include "RenderGraphUtils.h"
#include "Logs/RiveRendererLog.h"
#include "RiveRenderer.h"
#include <Metal/Metal.h>

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/renderer.hpp"
#include "rive/renderer/metal/render_context_metal_impl.h"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE
#include "Mac/AutoreleasePool.h"

FRiveRenderTargetMetal::FRiveRenderTargetMetal(
    const TSharedRef<FRiveRenderer>& InRiveRenderer,
    const FName& InRiveName,
    UTexture2DDynamic* InRenderTarget) :
    FRiveRenderTarget(InRiveRenderer, InRiveName, InRenderTarget)
{}

FRiveRenderTargetMetal::~FRiveRenderTargetMetal()
{
    RIVE_DEBUG_FUNCTION_INDENT
    RenderTargetMetal.reset();
}

DECLARE_GPU_STAT_NAMED(
    CacheTextureTarget,
    TEXT("FRiveRenderTargetMetal::CacheTextureTarget_RenderThread"));
void FRiveRenderTargetMetal::CacheTextureTarget_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    const FTextureRHIRef& InTexture)
{
    AutoreleasePool pool;
    check(IsInRenderingThread());

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

#if WITH_RIVE

    rive::gpu::RenderContext* RenderContext = RiveRenderer->GetRenderContext();

    if (RenderContext == nullptr)
    {
        return;
    }

#endif // WITH_RIVE

    // TODO, make sure we have correct texture DXGI_FORMAT_R8G8B8A8_UNORM
    EPixelFormat PixelFormat = InTexture->GetFormat();

    if (PixelFormat != PF_R8G8B8A8)
    {
        const FString PixelFormatString =
            GetPixelFormatString(InTexture->GetFormat());
        UE_LOG(LogRiveRenderer,
               Error,
               TEXT("Texture Target has the wrong Pixel Format '%s' (not "
                    "PF_R8G8B8A8)"),
               *PixelFormatString);

        return;
    }

    SCOPED_GPU_STAT(RHICmdList, CacheTextureTarget);

    const bool bIsValidRHI =
        GDynamicRHI != nullptr &&
        GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Metal;

    if (bIsValidRHI && InTexture.IsValid())
    {
        // Get the underlying metal texture so we can render to it.
        id<MTLTexture> MetalTexture =
            (id<MTLTexture>)InTexture->GetNativeResource();

        check(MetalTexture);
        NSUInteger Format = MetalTexture.pixelFormat;
        UE_LOG(LogRiveRenderer,
               Verbose,
               TEXT("MetalTexture texture %dx%d"),
               MetalTexture.width,
               MetalTexture.height);
        UE_LOG(LogRiveRenderer,
               Verbose,
               TEXT("  format: %d (MTLPixelFormatRGBA8Unorm_sRGB: %d, "
                    "MTLPixelFormatRGBA8Unorm: %d)"),
               Format,
               MTLPixelFormatRGBA8Unorm_sRGB,
               MTLPixelFormatRGBA8Unorm);
        UE_LOG(LogRiveRenderer,
               Verbose,
               TEXT("  format: %d (MTLPixelFormatBGRA8Unorm_sRGB: %d, "
                    "MTLPixelFormatBGRA8Unorm: %d)"),
               Format,
               MTLPixelFormatBGRA8Unorm_sRGB,
               MTLPixelFormatBGRA8Unorm);

#if WITH_RIVE
        if (RenderTargetMetal)
        {
            RenderTargetMetal.reset();
        }

        // For now we just set one renderer and one texture
        rive::gpu::RenderContextMetalImpl* const RenderContextMetalImpl =
            RenderContext
                ->static_impl_cast<rive::gpu::RenderContextMetalImpl>();

        RenderTargetMetal =
            RenderContextMetalImpl->makeRenderTarget(MetalTexture.pixelFormat,
                                                     MetalTexture.width,
                                                     MetalTexture.height);

        RenderTargetMetal->setTargetTexture(MetalTexture);

#endif // WITH_RIVE
    }
}

#if WITH_RIVE
rive::rcp<rive::gpu::RenderTarget> FRiveRenderTargetMetal::GetRenderTarget()
    const
{
    return RenderTargetMetal;
}

void FRiveRenderTargetMetal::EndFrame() const
{
    rive::gpu::RenderContext* RenderContext = RiveRenderer->GetRenderContext();
    if (RenderContext == nullptr)
    {
        return;
    }

    // End drawing a frame.
    id<MTLCommandQueue> MetalCommandQueue =
        (id<MTLCommandQueue>)GDynamicRHI->RHIGetNativeGraphicsQueue();
    id<MTLCommandBuffer> flushCommandBuffer = [MetalCommandQueue commandBuffer];
    rive::gpu::RenderContext::FlushResources FlushResources{
        GetRenderTarget().get(),
        (__bridge void*)flushCommandBuffer};
    RenderContext->flush(FlushResources);
    [flushCommandBuffer commit];
}
#endif // WITH_RIVE

#endif // PLATFORM_APPLE
