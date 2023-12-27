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

void UE::Rive::Renderer::Private::FRiveRenderer::QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile)
{
}

void UE::Rive::Renderer::Private::FRiveRenderer::DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor)
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
