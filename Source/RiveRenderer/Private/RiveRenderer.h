// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderer.h"

namespace UE::Rive::Renderer
{
    class FRiveRenderTarget;
}

namespace UE::Rive::Renderer::Private
{
    class FRiveRenderer : public IRiveRenderer
    {
        //~ BEGIN : IRiveRenderer Interface

    public:

        FRiveRenderer();

        virtual ~FRiveRenderer();

        virtual void QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile) override;

        virtual void DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor) override;

        virtual void SetTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) override {}

        virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override {}

        virtual void CreatePLSContext() override {}

        virtual void CreatePLSRenderer() override {}

        //~ END : IRiveRenderer Interface

    protected:
        //std::unique_ptr<rive::pls::PLSRenderContext> m_plsContext;

        TMap<FName, TSharedPtr<FRiveRenderTarget>> RenderTargets;

    protected:
        mutable FCriticalSection ThreadDataCS;
    };
}
