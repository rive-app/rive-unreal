#pragma once
#include "RiveRenderer.h"

class RIVERENDERER_API FRiveRendererRHI : public FRiveRenderer
{
public:
    //~ BEGIN : IRiveRenderer Interface
    virtual TSharedPtr<IRiveRenderTarget> CreateTextureTarget_GameThread(
        const FName& InRiveName,
        UTexture2DDynamic* InRenderTarget) override;
    virtual void CreateRenderContext_RenderThread(
        FRHICommandListImmediate& RHICmdList) override;
    virtual void Flush(rive::gpu::RenderContext& context) {}
    //~ END : IRiveRenderer Interface
};
