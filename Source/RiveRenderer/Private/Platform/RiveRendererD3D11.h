// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RiveRenderer.h"
#include "rive/refcnt.hpp"

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
		
		virtual void SetTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) override;

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override;

		virtual void CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList) override;

		virtual void CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList) override;

	private:
		rive::rcp<rive::pls::PLSRenderTargetD3D> TestPLSRenderTargetD3D;

	};
}

