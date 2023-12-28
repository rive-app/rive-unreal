// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveRenderTarget.h"
#include "rive/refcnt.hpp"

namespace rive::pls
{
	class PLSRenderTargetD3D;
}

namespace UE::Rive::Renderer::Private
{
	class FRiveRenderTargetD3D11 : public FRiveRenderTarget
	{
	public:
		FRiveRenderTargetD3D11(const TSharedPtr<Private::FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget);
		
		virtual void Initialize() override;

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override;

		virtual void DrawArtboard(uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtBoard) override;

	protected:
		virtual void DrawArtboard_RenderThread(FRHICommandListImmediate& RHICmdList, uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtBoard) override;

		// It Might need to be on rendering thread, render QUEUE is required
		virtual std::unique_ptr<rive::pls::PLSRenderer> GetPLSRenderer() const override;
		
	private:
		rive::rcp<rive::pls::PLSRenderTargetD3D> CachedPLSRenderTargetD3D;

		mutable bool bIsCleared = false;
	};
}
