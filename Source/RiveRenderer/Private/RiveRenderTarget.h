// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"

#if WITH_RIVE

namespace rive::pls
{
	class PLSRenderer;
}

#endif // WITH_RIVE

class UTextureRenderTarget2D;

namespace UE::Rive::Renderer::Private
{
	class FRiveRenderer;

	class FRiveRenderTarget : public IRiveRenderTarget
	{
		/**
		 * Structor(s)
		 */

	public:

		FRiveRenderTarget(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget);
		
		//~ BEGIN : IRiveRenderTarget Interface

	public:

		virtual void Initialize() override {}

#if WITH_RIVE

		virtual void DrawArtboard(uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor) override
		{
		}

#endif // WITH_RIVE

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override {}
		
		virtual uint32 GetWidth() const;
		
		virtual uint32 GetHeight() const;

		virtual FCriticalSection& GetThreadDataCS() { return ThreadDataCS; }
	
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

		mutable FCriticalSection ThreadDataCS;

		TSharedPtr<FRiveRenderer> RiveRenderer;
		
		FName RiveName;
		
		TObjectPtr<UTextureRenderTarget2D> RenderTarget;

		mutable FDateTime LastResetTime = FDateTime::Now();

		static FTimespan ResetTimeLimit;
	};
}
