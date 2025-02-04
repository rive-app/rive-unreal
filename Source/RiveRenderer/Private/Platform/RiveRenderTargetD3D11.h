// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RiveRenderTarget.h"

#if PLATFORM_WINDOWS

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/refcnt.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive::gpu
{
class RenderTargetD3D;
}

#endif // WITH_RIVE

class FRiveRendererD3D11;

class FRiveRenderTargetD3D11 final : public FRiveRenderTarget
{
public:
    /**
     * Structor(s)
     */

    FRiveRenderTargetD3D11(const TSharedRef<FRiveRendererD3D11>& InRiveRenderer,
                           const FName& InRiveName,
                           UTexture2DDynamic* InRenderTarget);
    virtual ~FRiveRenderTargetD3D11() override;

    //~ BEGIN : IRiveRenderTarget Interface
public:
    virtual void CacheTextureTarget_RenderThread(
        FRHICommandListImmediate& RHICmdList,
        const FTextureRHIRef& InRHIResource) override;
    //~ END : IRiveRenderTarget Interface

#if WITH_RIVE
    //~ BEGIN : FRiveRenderTarget Interface
protected:
    // It Might need to be on rendering thread, render QUEUE is required
    virtual void Render_RenderThread(
        FRHICommandListImmediate& RHICmdList,
        const TArray<FRiveRenderCommand>& RiveRenderCommands) override;
    virtual rive::rcp<rive::gpu::RenderTarget> GetRenderTarget() const override;
    //~ END : FRiveRenderTarget Interface

    /**
     * Attribute(s)
     */
private:
    TSharedRef<FRiveRendererD3D11> RiveRendererD3D11;
    rive::rcp<rive::gpu::RenderTargetD3D> RenderTargetD3D;
#endif // WITH_RIVE
};

#endif // PLATFORM_WINDOWS
