// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Logs/RiveRendererLog.h"
#include "TextureResource.h"

UE::Rive::Renderer::Private::FRiveRenderer::FRiveRenderer()
{
}

UE::Rive::Renderer::Private::FRiveRenderer::~FRiveRenderer()
{
}

void UE::Rive::Renderer::Private::FRiveRenderer::Initialize()
{
    check(IsInGameThread());
    
    FScopeLock Lock(&ThreadDataCS);

    ENQUEUE_RENDER_COMMAND(FRiveRenderer_Initialize)(
    [this](FRHICommandListImmediate& RHICmdList)
    {
        CreatePLSContext_RenderThread(RHICmdList);

        CreatePLSRenderer_RenderThread(RHICmdList);
    });

    // Should give more data how that was initialized
    bIsInitialized = true;
}


void UE::Rive::Renderer::Private::FRiveRenderer::QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile)
{
}

#if WITH_RIVE

rive::pls::PLSRenderContext* UE::Rive::Renderer::Private::FRiveRenderer::GetPLSRenderContextPtr()
{
    if (!PLSRenderContext)
    {
        UE_LOG(LogRiveRenderer, Error, TEXT("Rive PLS Render Context is uninitialized."));

        return nullptr;
    }

    return PLSRenderContext.get();
}

rive::pls::PLSRenderer* UE::Rive::Renderer::Private::FRiveRenderer::GetPLSRendererPtr()
{
    if (!PLSRenderer)
    {
        UE_LOG(LogRiveRenderer, Error, TEXT("Rive PLS Renderer is uninitialized."));

        return nullptr;
    }

    return PLSRenderer.get();
}

#endif // WITH_RIVE

UTextureRenderTarget2D* UE::Rive::Renderer::Private::FRiveRenderer::CreateDefaultRenderTarget(FIntPoint InTargetSize)
{
    UTextureRenderTarget2D* const RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());

    RenderTarget->OverrideFormat = EPixelFormat::PF_R8G8B8A8;

    RenderTarget->bCanCreateUAV = true;
    
    RenderTarget->ResizeTarget(InTargetSize.X, InTargetSize.Y);

    RenderTarget->UpdateResourceImmediate(true);

    FlushRenderingCommands();

    return RenderTarget;
}
