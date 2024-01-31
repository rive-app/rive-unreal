// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderer.h"

#if WITH_RIVE

namespace rive::pls
{
    class PLSRenderer;
    class PLSRenderContext;
}

#endif // WITH_RIVE

namespace UE::Rive::Renderer::Private
{
    class FRiveRenderTarget;
    
    class FRiveRenderer : public IRiveRenderer
    {
        /**
         * Structor(s)
         */

    public:

        FRiveRenderer();

        virtual ~FRiveRenderer();

        //~ BEGIN : IRiveRenderer Interface
    
    public:

        virtual void Initialize() override;

        virtual bool IsInitialized() const override { return bIsInitialized; }

        virtual void QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile) override;

        virtual IRiveRenderTargetPtr CreateTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) override { return nullptr; }

        virtual UTextureRenderTarget2D* CreateDefaultRenderTarget(FIntPoint InTargetSize) override;

#if WITH_RIVE

        virtual rive::pls::PLSRenderContext* GetPLSRenderContextPtr() override;
        
        virtual rive::pls::PLSRenderer* GetPLSRendererPtr() override;

#endif // WITH_RIVE

        //~ END : IRiveRenderer Interface

        /**
         * Attribute(s)
         */

    protected:

#if WITH_RIVE

        std::unique_ptr<rive::pls::PLSRenderContext> PLSRenderContext;
        
        std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer;

#endif // WITH_RIVE

        TMap<FName, TSharedPtr<FRiveRenderTarget>> RenderTargets;

    protected:

        mutable FCriticalSection ThreadDataCS;

    private:

        bool bIsInitialized = false;
    };
}
