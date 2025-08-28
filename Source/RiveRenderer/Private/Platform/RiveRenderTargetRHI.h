// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once
#include "RenderContextRHIImpl.hpp"
#include "RiveRenderTarget.h"

class FRiveRendererRHI;

class FRiveRenderTargetRHI final : public FRiveRenderTarget
{
public:
    FRiveRenderTargetRHI(FRiveRenderer* Renderer,
                         const FName& InRiveName,
                         UTexture2DDynamic* InRenderTarget);

    FRiveRenderTargetRHI(FRiveRenderer* Renderer,
                         const FName& InRiveName,
                         UTextureRenderTarget2D* InRenderTarget);

    FRiveRenderTargetRHI(FRiveRenderer* Renderer,
                         const FName& InRiveName,
                         FRenderTarget* InRenderTarget);

    FRiveRenderTargetRHI(FRDGBuilder& GraphBuilder,
                         FRiveRenderer* Renderer,
                         const FName& InRiveName,
                         FRDGTextureRef InRenderTarget);

    virtual ~FRiveRenderTargetRHI() override;

    virtual void CacheTextureTarget_RenderThread(
        FRHICommandListImmediate& RHICmdList,
        const FTextureRHIRef& InRHIResource) override;

    virtual void UpdateTargetTexture(FRDGTextureRef InRenderTarget) override;
    virtual void UpdateTargetTexture(FRenderTarget* InRenderTarget) override;
    virtual void UpdateRHIResorourceDirect(
        FTextureRHIRef InRenderTarget) override;
    virtual rive::rcp<rive::gpu::RenderTarget> GetRenderTarget() const override
    {
        return CachedRenderTarget;
    };

private:
    rive::rcp<RenderTargetRHI> CachedRenderTarget;
};
