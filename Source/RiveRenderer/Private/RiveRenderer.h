// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderer.h"

namespace rive::pls
{
    class PLSRenderer;
    class PLSRenderContext;
}

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

        virtual void Initialize() override;

        virtual bool IsInitialized() const override { return bIsInitialized; }

        virtual void QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile) override;

        virtual void DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor, rive::Artboard* InNativeArtBoard) override;

        virtual TSharedPtr<IRiveRenderTarget> CreateTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) override { return nullptr; }

        virtual UTextureRenderTarget2D* CreateDefaultRenderTarget(FIntPoint InTargetSize) override;

    public:
        rive::pls::PLSRenderContext* GetPLSRenderContextPtr() { return PLSRenderContext.get(); }
        
        //~ END : IRiveRenderer Interface

    protected:
        std::unique_ptr<rive::pls::PLSRenderContext> PLSRenderContext;
        
        //std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer;

        TMap<FName, TSharedPtr<FRiveRenderTarget>> RenderTargets;

    protected:
        mutable FCriticalSection ThreadDataCS;

    private:
        bool bIsInitialized = false;
    };
}
