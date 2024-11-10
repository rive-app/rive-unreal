// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PixelFormat.h"
#include "RHI.h"

class UTextureRenderTarget2D;
class FRHICommandListImmediate;

struct FRiveRendererUtils
{
    static RIVERENDERER_API UTextureRenderTarget2D* CreateDefaultRenderTarget(
        FIntPoint InTargetSize,
        EPixelFormat PixelFormat = EPixelFormat::PF_B8G8R8A8,
        bool bCanCreateUAV = false);

    static RIVERENDERER_API FIntPoint
    GetRenderTargetSize(const UTextureRenderTarget2D* InRenderTarget);

    static RIVERENDERER_API void CopyTextureRDG(
        FRHICommandListImmediate& RHICmdList,
        FTextureRHIRef SourceTexture,
        FTextureRHIRef DestTexture);
};