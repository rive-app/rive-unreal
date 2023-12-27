// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class URiveFile;
class UTextureRenderTarget2D;

// just for testig
namespace rive
{
    class Artboard;
}

namespace UE::Rive::Renderer
{
    class IRiveRenderer
    {
        /**
         * Structor(s)
         */

    public:

        virtual ~IRiveRenderer() = default;

        /**
         * Implementation(s)
         */

    public:
        virtual void Initialize() = 0;

        virtual bool IsInitialized() const = 0;
        
        virtual void QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile) = 0;

        // just  for testing, we should not pass native artboard
        virtual void DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor, rive::Artboard* InNativeArtBoard) = 0;

        virtual void SetTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) = 0;

        virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) = 0;

        virtual void CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList) = 0;

        virtual void CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList) = 0;
    };
}
