// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class URiveFile;
class UTextureRenderTarget2D;

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

        virtual void QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile) = 0;

        virtual void DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor) = 0;

        virtual void SetTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) = 0;

        virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) = 0;

        virtual void CreatePLSContext() = 0;

        virtual void CreatePLSRenderer() = 0;
    };
}
