// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once
#include "RiveRenderer.h"

class RIVERENDERER_API FRiveRendererRHI : public FRiveRenderer
{
public:
    FRiveRendererRHI();

    virtual TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        const FName& InRiveName,
        UTexture2DDynamic* InRenderTarget) override;

    virtual TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        const FName& InRiveName,
        UTextureRenderTarget2D* InRenderTarget) override;

    virtual TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        FRDGBuilder& GraphBuilder,
        const FName& InRiveName,
        FRDGTextureRef InRenderTarget) override;

    virtual TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        const FName& InRiveName,
        FRenderTarget* RenderTarget) override;

    virtual void CreateRenderContext(
        FRHICommandListImmediate& RHICmdList) override;

    virtual void Flush(rive::gpu::RenderContext& context) {}
};
