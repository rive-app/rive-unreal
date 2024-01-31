// Copyright Rive, Inc. All rights reserved.

#pragma once

class UTextureRenderTarget2D;
class FRHICommandListImmediate;

namespace UE::Rive::Renderer
{
    struct FRiveRendererUtils
    {
        static RIVERENDERER_API UTextureRenderTarget2D* CreateDefaultRenderTarget(FIntPoint InTargetSize, EPixelFormat PixelFormat = EPixelFormat::PF_B8G8R8A8, bool bCanCreateUAV = false);

        static RIVERENDERER_API FIntPoint GetRenderTargetSize(const UTextureRenderTarget2D* InRenderTarget);

        static RIVERENDERER_API void CopyTextureRDG(FRHICommandListImmediate& RHICmdList, FTextureRHIRef SourceTexture, FTextureRHIRef DestTexture);
    };
}
