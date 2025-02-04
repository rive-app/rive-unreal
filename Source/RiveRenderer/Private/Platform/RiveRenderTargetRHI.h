#pragma once
#include "RenderContextRHIImpl.hpp"
#include "RiveRenderTarget.h"

class FRiveRendererRHI;

class FRiveRenderTargetRHI final : public FRiveRenderTarget
{
public:
    FRiveRenderTargetRHI(const TSharedRef<FRiveRendererRHI>& InRiveRenderer,
                         const FName& InRiveName,
                         UTexture2DDynamic* InRenderTarget);
    virtual ~FRiveRenderTargetRHI() override;

    //~ BEGIN : IRiveRenderTarget Interface
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
#endif // WITH_RIVE
private:
    TSharedRef<FRiveRendererRHI> RiveRenderer;
    rive::rcp<RenderTargetRHI> CachedRenderTarget;
};
