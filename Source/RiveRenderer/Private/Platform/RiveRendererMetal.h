// Copyright Rive, Inc. All rights reserved.

#pragma once
#include "CoreMinimal.h"

#if PLATFORM_APPLE

#include "RiveRenderer.h"
/**
 *
 */
class RIVERENDERER_API FRiveRendererMetal : public FRiveRenderer
{
    /**
     * Structor(s)
     */

public:
    //~ BEGIN : IRiveRenderer Interface

public:
    virtual TSharedPtr<IRiveRenderTarget> CreateTextureTarget_GameThread(
        const FName& InRiveName,
        UTexture2DDynamic* InRenderTarget) override;

    virtual void CreateRenderContext_RenderThread(
        FRHICommandListImmediate& RHICmdList) override;

    //~ END : IRiveRenderer Interface
};

#endif // PLATFORM_APPLE
