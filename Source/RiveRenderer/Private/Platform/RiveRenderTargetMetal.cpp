// Copyright Rive, Inc. All rights reserved.

#include "Platform/RiveRenderTargetMetal.h"

#if PLATFORM_APPLE

#include "DynamicRHI.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Logs/RiveRendererLog.h"
#include "RiveRenderer.h"
#include <Metal/Metal.h>

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/metal/pls_render_context_metal_impl.h"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRenderTargetMetal::FRiveRenderTargetMetal(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
    : FRiveRenderTarget(InRiveRenderer, InRiveName, InRenderTarget)
{
}

UE::Rive::Renderer::Private::FRiveRenderTargetMetal::~FRiveRenderTargetMetal()
{
    RIVE_DEBUG_FUNCTION_INDENT
    CachedPLSRenderTargetMetal.release();
}

void UE::Rive::Renderer::Private::FRiveRenderTargetMetal::Initialize()
{
    check(IsInGameThread());
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
    
    ENQUEUE_RENDER_COMMAND(CacheTextureTarget_RenderThread)(
    [RenderTargetResource, this](FRHICommandListImmediate& RHICmdList)
    {
        CacheTextureTarget_RenderThread(RHICmdList, RenderTargetResource->TextureRHI->GetTexture2D());
    });
}

DECLARE_GPU_STAT_NAMED(CacheTextureTarget, TEXT("FRiveRenderTargetMetal::CacheTextureTarget_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRenderTargetMetal::CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InTexture)
{
    check(IsInRenderingThread());
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
#if WITH_RIVE
    
    rive::pls::PLSRenderContext* PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr();
    
    if (PLSRenderContext == nullptr)
    {
        return;
    }
    
#endif // WITH_RIVE
    
    // TODO, make sure we have correct texture DXGI_FORMAT_R8G8B8A8_UNORM
    EPixelFormat PixelFormat = InTexture->GetFormat();
    
    if (PixelFormat != PF_R8G8B8A8)
    {
        return;
    }
    
    SCOPED_GPU_STAT(RHICmdList, CacheTextureTarget);

    const bool bIsValidRHI = GDynamicRHI != nullptr && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Metal;

    if (bIsValidRHI && InTexture.IsValid())
    {        
        // Get the underlying metal texture so we can render to it.
        id<MTLTexture> MetalTexture = (id<MTLTexture>)InTexture->GetNativeResource();
        
        check(MetalTexture);
        
        UE_LOG(LogRiveRenderer, Warning, TEXT("MetalTexture texture %dx%d"), MetalTexture.width, MetalTexture.height);

#if WITH_RIVE
        
        // For now we just set one renderer and one texture
        rive::pls::PLSRenderContextMetalImpl* const PLSRenderContextMetalImpl = PLSRenderContext->static_impl_cast<rive::pls::PLSRenderContextMetalImpl>();
        
        CachedPLSRenderTargetMetal = PLSRenderContextMetalImpl->makeRenderTarget(MetalTexture.pixelFormat, MetalTexture.width, MetalTexture.height);
        
        CachedPLSRenderTargetMetal->setTargetTexture(MetalTexture);
        
#endif // WITH_RIVE
    }
}

#if WITH_RIVE


void UE::Rive::Renderer::Private::FRiveRenderTargetMetal::DrawArtboard(uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
    check(IsInGameThread());

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    ENQUEUE_RENDER_COMMAND(AlignArtboard)(
        [this, InFit, AlignX, AlignY, InNativeArtboard, DebugColor](FRHICommandListImmediate& RHICmdList)
        {
            DrawArtboard_RenderThread(RHICmdList, InFit, AlignX, AlignY, InNativeArtboard, DebugColor);
        });
}

DECLARE_GPU_STAT_NAMED(DrawArtboard, TEXT("FRiveRenderTargetMetal::DrawArtboard"));
void UE::Rive::Renderer::Private::FRiveRenderTargetMetal::DrawArtboard_RenderThread(FRHICommandListImmediate& RHICmdList, uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
    SCOPED_GPU_STAT(RHICmdList, DrawArtboard);

    check(IsInRenderingThread());

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();
    
    if (PLSRenderContextPtr == nullptr)
    {
        return;
    }
    
    // Begin Frame
    std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = GetPLSRenderer(DebugColor);

    if (PLSRenderer == nullptr)
    {
        return;
    }

    // Align Artboard
    const rive::Fit& Fit = *reinterpret_cast<rive::Fit*>(&InFit);

    const rive::Alignment& Alignment = rive::Alignment(AlignX, AlignY);

    const uint32 TextureWidth = GetWidth();

    const uint32 TextureHeight = GetHeight();

    rive::Mat2D Transform = rive::computeAlignment(
        Fit,
        Alignment,
        rive::AABB(0.f, 0.f, TextureWidth, TextureHeight),
        InNativeArtboard->bounds());

    PLSRenderer->transform(Transform);

    // Draw Artboard
    InNativeArtboard->draw(PLSRenderer.get());

    { // End drawing a frame.
        // Flush
        PLSRenderContextPtr->flush();

        const FDateTime Now = FDateTime::Now();

        const int32 TimeElapsed = (Now - LastResetTime).GetSeconds();

        if (TimeElapsed >= ResetTimeLimit.GetSeconds())
        {
            // Reset
            PLSRenderContextPtr->shrinkGPUResourcesToFit();

            PLSRenderContextPtr->resetGPUResources();

            LastResetTime = Now;
        }
    }
}

std::unique_ptr<rive::pls::PLSRenderer> UE::Rive::Renderer::Private::FRiveRenderTargetMetal::GetPLSRenderer(const FLinearColor DebugColor) const
{
    rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();
    
    if (PLSRenderContextPtr == nullptr)
    {
        return nullptr;
    }
    
    FColor ClearColor = DebugColor.ToRGBE();
    
    rive::pls::PLSRenderContext::FrameDescriptor FrameDescriptor;
    
    FrameDescriptor.renderTarget = CachedPLSRenderTargetMetal;
    
    FrameDescriptor.loadAction = bIsCleared ? rive::pls::LoadAction::clear : rive::pls::LoadAction::preserveRenderTarget;
    
    FrameDescriptor.clearColor = rive::colorARGB(ClearColor.A, ClearColor.R,ClearColor.G,ClearColor.B);
    
    FrameDescriptor.wireframe = false;
    
    FrameDescriptor.fillsDisabled = false;
    
    FrameDescriptor.strokesDisabled = false;
    
    if (bIsCleared == false)
    {
        bIsCleared = true;
    }
    
    // flush all the time
    PLSRenderContextPtr->beginFrame(std::move(FrameDescriptor));
    
    return std::make_unique<rive::pls::PLSRenderer>(PLSRenderContextPtr);;
}

#endif // WITH_RIVE

UE_ENABLE_OPTIMIZATION

#endif // PLATFORM_APPLE
