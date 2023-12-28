// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderer.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/TextureRenderTarget2D.h"
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

void UE::Rive::Renderer::Private::FRiveRenderer::DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor, rive::Artboard* InNativeArtBoard)
{
    check(InTexture);

    FTextureRenderTargetResource* RenderTargetResource = InTexture->GameThread_GetRenderTargetResource();

    ENQUEUE_RENDER_COMMAND(CopyRenderTexture)([this, RenderTargetResource, DebugColor](FRHICommandListImmediate& RHICmdList)
        {
            // JUST for testing here
            FCanvas Canvas(RenderTargetResource, nullptr, FGameTime(), GMaxRHIFeatureLevel);

            Canvas.Clear(DebugColor);
        });
}

UTextureRenderTarget2D* UE::Rive::Renderer::Private::FRiveRenderer::CreateDefaultRenderTarget(FIntPoint InTargetSize)
{
    UTextureRenderTarget2D* const RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());

    // RenderTarget->bForceLinearGamma = false;
    //
    // RenderTarget->bAutoGenerateMips = false;

    RenderTarget->OverrideFormat = EPixelFormat::PF_R8G8B8A8;
    RenderTarget->bCanCreateUAV = true;
    
    RenderTarget->ResizeTarget(InTargetSize.X, InTargetSize.Y);
    RenderTarget->UpdateResourceImmediate(true);

    FlushRenderingCommands();

    return RenderTarget;
}
