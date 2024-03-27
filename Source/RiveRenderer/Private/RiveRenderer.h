// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderer.h"
#include "RiveTypes.h"

#include <memory>

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

        virtual ~FRiveRenderer() override;

        //~ BEGIN : IRiveRenderer Interface
    
    public:

        virtual void Initialize() override;

        virtual bool IsInitialized() const override { return InitializationState == ERiveInitState::Initialized; }

        virtual void QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile) override;

        virtual IRiveRenderTargetPtr CreateTextureTarget_GameThread(const FName& InRiveName, UTexture2DDynamic* InRenderTarget) override { return nullptr; }

        virtual UTextureRenderTarget2D* CreateDefaultRenderTarget(FIntPoint InTargetSize) override;

        virtual FCriticalSection& GetThreadDataCS() override { return ThreadDataCS; }

        virtual void CallOrRegister_OnInitialized(FOnRendererInitialized::FDelegate&& Delegate) override;

#if WITH_RIVE

        virtual rive::pls::PLSRenderContext* GetPLSRenderContextPtr() override;

#endif // WITH_RIVE

        //~ END : IRiveRenderer Interface

        /**
         * Attribute(s)
         */

    protected:

#if WITH_RIVE

        std::unique_ptr<rive::pls::PLSRenderContext> PLSRenderContext;

#endif // WITH_RIVE

        TMap<FName, TSharedPtr<FRiveRenderTarget>> RenderTargets;

    protected:

        mutable FCriticalSection ThreadDataCS;

    protected:
        ERiveInitState InitializationState = ERiveInitState::Uninitialized;
        FOnRendererInitialized OnInitializedDelegate;
    };
}
