// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderer.h"
#include "RiveTypes.h"

#include <memory>

#if WITH_RIVE

namespace rive::gpu
{
class Renderer;
class RenderContext;
} // namespace rive::gpu

#endif // WITH_RIVE

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

    virtual bool IsInitialized() const override
    {
        return InitializationState == ERiveInitState::Initialized;
    }

    virtual TSharedPtr<IRiveRenderTarget> CreateTextureTarget_GameThread(
        const FName& InRiveName,
        UTexture2DDynamic* InRenderTarget) override
    {
        return nullptr;
    }

    virtual UTextureRenderTarget2D* CreateDefaultRenderTarget(
        FIntPoint InTargetSize) override;

    virtual FCriticalSection& GetThreadDataCS() override
    {
        return ThreadDataCS;
    }

    virtual void CallOrRegister_OnInitialized(
        FOnRendererInitialized::FDelegate&& Delegate) override;

#if WITH_RIVE

    virtual rive::gpu::RenderContext* GetRenderContext() override;

#endif // WITH_RIVE

    //~ END : IRiveRenderer Interface

    /**
     * Attribute(s)
     */

protected:
#if WITH_RIVE

    std::unique_ptr<rive::gpu::RenderContext> RenderContext;

#endif // WITH_RIVE

    TMap<FName, TSharedPtr<FRiveRenderTarget>> RenderTargets;

protected:
    mutable FCriticalSection ThreadDataCS;

protected:
    ERiveInitState InitializationState = ERiveInitState::Uninitialized;
    FOnRendererInitialized OnInitializedDelegate;
};
