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
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/renderer/metal/render_context_metal_impl.h"
#include "rive/renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE
#include "Mac/AutoreleasePool.h"

TSharedPtr<IRiveRenderTarget> FRiveRendererMetal::
    CreateTextureTarget_GameThread(const FName& InRiveName,
                                   UTexture2DDynamic* InRenderTarget)
{
    check(IsInGameThread());

    FScopeLock Lock(&ThreadDataCS);

    const TSharedPtr<FRiveRenderTargetMetal> RiveRenderTarget =
        MakeShared<FRiveRenderTargetMetal>(SharedThis(this),
                                           InRiveName,
                                           InRenderTarget);

    RenderTargets.Add(InRiveName, RiveRenderTarget);

    return RiveRenderTarget;
}

DECLARE_GPU_STAT_NAMED(CreateRenderContext,
                       TEXT("CreateRenderContext_RenderThread"));
void FRiveRendererMetal::CreateRenderContext_RenderThread(
    FRHICommandListImmediate& RHICmdList)
{
    AutoreleasePool pool;
    check(IsInRenderingThread());

    FScopeLock Lock(&ThreadDataCS);

    SCOPED_GPU_STAT(RHICmdList, CreateRenderContext);

    if (GDynamicRHI != nullptr &&
        GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Metal)
    {
        // Get the underlying metal device.
        id<MTLDevice> MetalDevice =
            (id<MTLDevice>)GDynamicRHI->RHIGetNativeDevice();
        check(MetalDevice);

#if WITH_RIVE
        RenderContext =
            rive::gpu::RenderContextMetalImpl::MakeContext(MetalDevice);
#endif // WITH_RIVE
    }
}

#endif // PLATFORM_APPLE
