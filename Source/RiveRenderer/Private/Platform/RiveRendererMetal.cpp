// Copyright Rive, Inc. All rights reserved.

#include "Platform/RiveRendererMetal.h"

#if PLATFORM_APPLE

#include "CanvasTypes.h"
#include "DynamicRHI.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RiveRenderTargetMetal.h"
#include <Metal/Metal.h>

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/metal/pls_render_context_metal_impl.h"
#include "rive/pls/pls_renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRendererMetal::FRiveRendererMetal()
{
}

UE::Rive::Renderer::Private::FRiveRendererMetal::~FRiveRendererMetal()
{
}


TSharedPtr<UE::Rive::Renderer::IRiveRenderTarget> UE::Rive::Renderer::Private::FRiveRendererMetal::CreateTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
{
    check(IsInGameThread());

    FScopeLock Lock(&ThreadDataCS);
    
    const TSharedPtr<FRiveRenderTargetMetal> RiveRenderTarget = MakeShared<FRiveRenderTargetMetal>(SharedThis(this), InRiveName, InRenderTarget);
    
    RenderTargets.Add(InRiveName, RiveRenderTarget);
    
    return RiveRenderTarget;
}

DECLARE_GPU_STAT_NAMED(CreatePLSContext, TEXT("CreatePLSContext_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererMetal::CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList)
{
    check(IsInRenderingThread());
    
    FScopeLock Lock(&ThreadDataCS);
    
    SCOPED_GPU_STAT(RHICmdList, CreatePLSContext);
    
    if (GDynamicRHI != nullptr && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Metal)
    {
        // Get the underlying metal device.
        id<MTLDevice> MetalDevice = (id<MTLDevice>)GDynamicRHI->RHIGetNativeDevice();
        
        check(MetalDevice);
        
        id<MTLCommandQueue> MetalCommandQueue = (id<MTLCommandQueue>)GDynamicRHI->RHIGetNativeGraphicsQueue();
        
        check(MetalCommandQueue);

#if WITH_RIVE
        
        PLSRenderContext = rive::pls::PLSRenderContextMetalImpl::MakeContext(MetalDevice, MetalCommandQueue);
        
#endif // WITH_RIVE
    }
}

DECLARE_GPU_STAT_NAMED(CreatePLSRenderer, TEXT("CreatePLSRenderer_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererMetal::CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList)
{
#if WITH_RIVE
    
    if (PLSRenderContext == nullptr)
    {
        return;
    }
    
    check(IsInRenderingThread());
    
    FScopeLock Lock(&ThreadDataCS);
    
    SCOPED_GPU_STAT(RHICmdList, CreatePLSRenderer);
    
    PLSRenderer = std::make_unique<rive::pls::PLSRenderer>(PLSRenderContext.get());
    
#endif // WITH_RIVE
}

UE_ENABLE_OPTIMIZATION

#endif // PLATFORM_APPLE
