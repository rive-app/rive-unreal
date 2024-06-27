#pragma once
#include "RiveRenderer.h"


class RIVERENDERER_API FRiveRendererRHI : public FRiveRenderer
{
public:
    //~ BEGIN : IRiveRenderer Interface
    virtual TSharedPtr<IRiveRenderTarget> CreateTextureTarget_GameThread(const FName& InRiveName, UTexture2DDynamic* InRenderTarget) override;
    virtual void CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList) override;
    //~ END : IRiveRenderer Interface
};
