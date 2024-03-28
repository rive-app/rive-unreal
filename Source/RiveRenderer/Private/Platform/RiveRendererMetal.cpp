// Copyright Rive, Inc. All rights reserved.

#include "Platform/RiveRendererMetal.h"

#if PLATFORM_APPLE

#include "CanvasTypes.h"
#include "DynamicRHI.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#include "RiveRenderTargetMetal.h"
#include <Metal/Metal.h>

#if WITH_RIVE
#include "RiveCore/Public/PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/metal/pls_render_context_metal_impl.h"
#include "rive/pls/pls_renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE
#include "Mac/AutoreleasePool.h"

TSharedPtr<UE::Rive::Renderer::IRiveRenderTarget> UE::Rive::Renderer::Private::FRiveRendererMetal::CreateTextureTarget_GameThread(const FName& InRiveName, UTexture2DDynamic* InRenderTarget)
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
    AutoreleasePool pool;
    check(IsInRenderingThread());
    
    FScopeLock Lock(&ThreadDataCS);
    
    SCOPED_GPU_STAT(RHICmdList, CreatePLSContext);
    
    if (GDynamicRHI != nullptr && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Metal)
    {
        // Get the underlying metal device.
        id<MTLDevice> MetalDevice = (id<MTLDevice>)GDynamicRHI->RHIGetNativeDevice();
        check(MetalDevice);

#if WITH_RIVE
        PLSRenderContext = rive::pls::PLSRenderContextMetalImpl::MakeContext(MetalDevice);
#endif // WITH_RIVE
    }
}

#endif // PLATFORM_APPLE
