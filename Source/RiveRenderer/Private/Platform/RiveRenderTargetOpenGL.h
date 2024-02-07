// Copyright Rive, Inc. All rights reserved.

#pragma once

// #undef PLATFORM_ANDROID
// #define PLATFORM_ANDROID 1
#if PLATFORM_ANDROID
#include "RiveRenderTarget.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/refcnt.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive::pls
{
	class TextureRenderTargetGL;
}

#endif // WITH_RIVE

namespace UE::Rive::Renderer::Private
{
	class FRiveRendererOpenGL;

	class FRiveRenderTargetOpenGL final : public FRiveRenderTarget
	{
		/**
		 * Structor(s)
		 */
	public:
		FRiveRenderTargetOpenGL(const TSharedRef<FRiveRendererOpenGL>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget);
		virtual ~FRiveRenderTargetOpenGL() override;
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
		virtual std::unique_ptr<rive::pls::PLSRenderer> GetPLSRenderer(const FLinearColor DebugColor) const override
		{
			check(false); // we don't want this called from any thread
			return nullptr;
		}
		//~ END : FRiveRenderTarget Interface
		
	private:
		virtual std::unique_ptr<rive::pls::PLSRenderer> GetPLSRenderer_Internal(const FLinearColor DebugColor) const;
		virtual void CacheTextureTarget_Internal(const FTexture2DRHIRef& InRHIResource);
		virtual void DrawArtboard_Internal(uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor& DebugColor);
		
		/**
		 * Attribute(s)
		 */

	
		rive::rcp<rive::pls::TextureRenderTargetGL> CachedPLSRenderTargetOpenGL;
		TSharedPtr<FRiveRendererOpenGL> RiveRendererGL;

#endif // WITH_RIVE

		mutable bool bIsCleared = false;
	};
}
#endif