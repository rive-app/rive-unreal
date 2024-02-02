// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RiveRenderTarget.h"

#if PLATFORM_WINDOWS

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/refcnt.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive::pls
{
	class PLSRenderTargetD3D;
}

#endif // WITH_RIVE

namespace UE::Rive::Renderer::Private
{
	class FRiveRenderTargetD3D11 final : public FRiveRenderTarget
	{
		/**
		 * Structor(s)
		 */

	public:

		FRiveRenderTargetD3D11(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget);

		//~ BEGIN : IRiveRenderTarget Interface

	public:

		virtual void Initialize() override;

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override;

#if WITH_RIVE

		virtual void DrawArtboard(uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor) override;

		//~ END : IRiveRenderTarget Interface

		//~ BEGIN : FRiveRenderTarget Interface

	protected:

		virtual void DrawArtboard_RenderThread(FRHICommandListImmediate& RHICmdList, uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor) override;

		// It Might need to be on rendering thread, render QUEUE is required
		virtual std::unique_ptr<rive::pls::PLSRenderer> GetPLSRenderer(const FLinearColor DebugColor) const override;
		
		//~ END : FRiveRenderTarget Interface
		
		/**
		 * Attribute(s)
		 */

	private:

		rive::rcp<rive::pls::PLSRenderTargetD3D> CachedPLSRenderTargetD3D;

#endif // WITH_RIVE

		mutable bool bIsCleared = false;
	};
}

#endif // PLATFORM_WINDOWS
