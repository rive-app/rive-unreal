// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UnrealClient.h"
#include "RenderGraphResources.h"

#if WITH_RIVE

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/refcnt.hpp"
#include "rive/command_queue.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive
{
class Renderer;

namespace gpu
{
class RenderContext;
class RenderTarget;
} // namespace gpu

class RiveRenderer;
} // namespace rive

#endif // WITH_RIVE

class UTexture2DDynamic;

class FRiveRenderer;

class FRiveRenderTarget
{
public:
    FRiveRenderTarget(FRiveRenderer* Renderer,
                      const FName& InRiveName,
                      UTexture2DDynamic* InRenderTarget);
    FRiveRenderTarget(FRiveRenderer* Renderer,
                      const FName& InRiveName,
                      UTextureRenderTarget2D* InRenderTarget);
    FRiveRenderTarget(FRiveRenderer* Renderer,
                      const FName& InRiveName,
                      FRenderTarget* InRenderTarget);
    FRiveRenderTarget(FRiveRenderer* Renderer,
                      const FName& InRiveName,
                      FRDGTextureRef InRenderTarget);
    virtual ~FRiveRenderTarget();

    virtual void Initialize();

    virtual void UpdateTargetTexture(UTexture2DDynamic* InRenderTarget);
    virtual void UpdateTargetTexture(FRDGTextureRef InRenderTarget);
    virtual void UpdateTargetTexture(FRenderTarget* InRenderTarget);
    virtual void UpdateRHIResorourceDirect(FTextureRHIRef InRenderTarget);

    virtual void CacheTextureTarget_RenderThread(
        FRHICommandListImmediate& RHICmdList,
        const FTextureRHIRef& InRHIResource)
    {
        RHIResource = InRHIResource;
    }

    TUniquePtr<rive::Renderer> BeginRenderFrame(
        rive::gpu::RenderContext* RenderContextPtr);
    virtual void EndRenderFrame(rive::gpu::RenderContext* RenderContextPtr);

    uint32 GetWidth() const;
    uint32 GetHeight() const;

    void SetClearRenderTarget(bool InClearRenderTarget)
    {
        bClearRenderTarget = InClearRenderTarget;
    }

    virtual rive::rcp<rive::gpu::RenderTarget> GetRenderTarget() const = 0;

protected:
    FName RiveName;
    FTextureRHIRef RHIResource;
    FRiveRenderer* RiveRenderer = nullptr;

    // Generic Render target used for UTextures
    TObjectPtr<UTexture2DDynamic> RenderTarget = nullptr;

    // Used for custom render to textures
    TObjectPtr<UTextureRenderTarget2D> RenderToTextureTarget = nullptr;

    // Used for thumbnail renderer
    FRenderTarget* ThumbnailRenderTarget = nullptr;

    // Used for rendering to UMG directly
    FRDGTextureRef RenderTargetRDG = nullptr;

    bool bClearRenderTarget = false;
};
