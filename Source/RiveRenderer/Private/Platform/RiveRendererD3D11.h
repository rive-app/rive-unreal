// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RiveRenderer.h"

namespace UE::Rive::Renderer::Private
{
	class RIVERENDERER_API FRiveRendererD3D11 : public FRiveRenderer
	{
	public:
		FRiveRendererD3D11();

		~FRiveRendererD3D11();
		
		virtual void SetTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) override;

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override;

		virtual void CreatePLSContext() override;

		virtual void CreatePLSRenderer() override;
	};
}

