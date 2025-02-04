// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_APPLE

#include "RiveRenderTarget.h"

#if WITH_RIVE
namespace rive::gpu
{
class RenderTargetMetal;
}

THIRD_PARTY_INCLUDES_START
#include "rive/refcnt.hpp"
THIRD_PARTY_INCLUDES_END

#endif // WITH_RIVE

/**
 *
 */
class FRiveRenderTargetMetal final : public FRiveRenderTarget
{
    /**
     * Structor(s)
     */

public:
    FRiveRenderTargetMetal(const TSharedRef<FRiveRenderer>& InRiveRenderer,
                           const FName& InRiveName,
                           UTexture2DDynamic* InRenderTarget);
    virtual ~FRiveRenderTargetMetal() override;
    //~ BEGIN : IRiveRenderTarget Interface

    virtual void CacheTextureTarget_RenderThread(
        FRHICommandListImmediate& RHICmdList,
        const FTextureRHIRef& InRHIResource) override;

#if WITH_RIVE

    //~ END : IRiveRenderTarget Interface

    //~ BEGIN : FRiveRenderTarget Interface

protected:
    virtual rive::rcp<rive::gpu::RenderTarget> GetRenderTarget() const override;
    virtual void EndFrame() const override;
    //~ END : FRiveRenderTarget Interface

    /**
     * Attribute(s)
     */

private:
    rive::rcp<rive::gpu::RenderTargetMetal> RenderTargetMetal;

#endif // WITH_RIVE

    mutable bool bIsCleared = false;
};

#endif // PLATFORM_APPLE
