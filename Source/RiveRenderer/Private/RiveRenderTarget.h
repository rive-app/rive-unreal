// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"

#if WITH_RIVE

namespace rive::pls
{
	class PLSRenderer;
}

#endif // WITH_RIVE

class UTexture2DDynamic;

namespace UE::Rive::Renderer::Private
{
	class FRiveRenderer;

	class FRiveRenderTarget : public IRiveRenderTarget
	{
		/**
		 * Structor(s)
		 */

	public:

		FRiveRenderTarget(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTexture2DDynamic* InRenderTarget);
		virtual ~FRiveRenderTarget() override;
		//~ BEGIN : IRiveRenderTarget Interface

	public:

		virtual void Initialize() override {}

#if WITH_RIVE

		virtual void DrawArtboard(uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor) override
		{
		}

#endif // WITH_RIVE

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override {}
		
		virtual uint32 GetWidth() const override;
		
		virtual uint32 GetHeight() const override;
	
		//~ END : IRiveRenderTarget Interface

		/**
		 * Implementation(s)
		 */

#if WITH_RIVE

	protected:

		virtual void DrawArtboard_RenderThread(FRHICommandListImmediate& RHICmdList, uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor) = 0;

		// It Might need to be on rendering thread, render QUEUE is required
		virtual std::unique_ptr<rive::pls::PLSRenderer> GetPLSRenderer(const FLinearColor DebugColor) const = 0;

#endif // WITH_RIVE

		/**
		 * Attribute(s)
		 */

	protected:
		TSharedPtr<FRiveRenderer> RiveRenderer;
		
		FName RiveName;
		
		TObjectPtr<UTexture2DDynamic> RenderTarget;

		mutable FDateTime LastResetTime = FDateTime::Now();

		static FTimespan ResetTimeLimit;
	};
}
