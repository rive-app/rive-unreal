#pragma once
#include "pls_render_context_rhi_impl.hpp"
#include "RiveRenderTarget.h"

class FRiveRendererRHI;

class FRiveRenderTargetRHI final: public FRiveRenderTarget
{
public:
    FRiveRenderTargetRHI(const TSharedRef<FRiveRendererRHI>& InRiveRenderer, const FName& InRiveName, UTexture2DDynamic* InRenderTarget);
    virtual ~FRiveRenderTargetRHI() override;
		
    //~ BEGIN : IRiveRenderTarget Interface
    virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override;
    //~ END : IRiveRenderTarget Interface
		
#if WITH_RIVE
    //~ BEGIN : FRiveRenderTarget Interface
protected:
    // It Might need to be on rendering thread, render QUEUE is required
    virtual void Render_RenderThread(FRHICommandListImmediate& RHICmdList, const TArray<FRiveRenderCommand>& RiveRenderCommands) override;
    virtual rive::rcp<rive::pls::PLSRenderTarget> GetRenderTarget() const override;
    //~ END : FRiveRenderTarget Interface
#endif // WITH_RIVE
private:
    TSharedRef<FRiveRendererRHI> RiveRenderer;
    rive::rcp<rive::pls::PLSRenderTargetRHI> CachedPLSRenderTarget;
};