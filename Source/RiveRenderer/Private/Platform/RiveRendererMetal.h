// Copyright Rive, Inc. All rights reserved.

#pragma once

#if PLATFORM_APPLE

#include "RiveRenderer.h"

#if WITH_RIVE

namespace rive::pls
{
    class PLSRenderTargetMetal;
    
    class PLSRenderContextMetalImpl;
}

#endif // WITH_RIVE

namespace UE::Rive::Renderer::Private
{
    /**
     *
     */
    class RIVERENDERER_API FRiveRendererMetal : public FRiveRenderer
    {
        /**
         * Structor(s)
         */
        
    public:
        
        FRiveRendererMetal();
        
        ~FRiveRendererMetal();
        
        //~ BEGIN : IRiveRenderer Interface
        
    public:
        
#if WITH_RIVE
        
        virtual void DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor, rive::Artboard* InNativeArtboard) override;
        
#endif // WITH_RIVE
        
        virtual IRiveRenderTargetPtr CreateTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) override;
        
        virtual void CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList) override;
        
        virtual void CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList) override;
        
        //~ END : IRiveRenderer Interface
    };
}

#endif // PLATFORM_APPLE
