// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RiveRenderer.h"

namespace rive::pls
{
	class PLSRenderTargetD3D;
	class PLSRenderContextD3DImpl;
}

namespace UE::Rive::Renderer::Private
{
	class RIVERENDERER_API FRiveRendererD3D11 : public FRiveRenderer
	{
	public:
		FRiveRendererD3D11();

		~FRiveRendererD3D11();

		virtual void DebugColorDraw(UTextureRenderTarget2D* InTexture, const FLinearColor DebugColor, rive::Artboard* InNativeArtBoard) override;
		
		virtual TSharedPtr<IRiveRenderTarget> CreateTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) override;
		
		virtual void CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList) override;

		virtual void CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList) override;
	};
}

