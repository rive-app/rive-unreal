// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IRiveRenderTarget.h"

namespace rive::pls
{
	class PLSRenderer;
}

class UTextureRenderTarget2D;

namespace UE::Rive::Renderer
{
	namespace Private
	{
		class FRiveRenderer;
	}

	class FRiveRenderTarget : public IRiveRenderTarget
	{
	public:
		FRiveRenderTarget(const TSharedPtr<Private::FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget);
		
		virtual void Initialize() override {}

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override {}

		virtual void AlignArtboard(uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtBoard) override;
		virtual void DrawArtboard(uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtBoard, const FLinearColor DebugColor) override {}

		virtual uint32 GetWidth() const;
		
		virtual uint32 GetHeight() const;
	
	protected:
		virtual void DrawArtboard_RenderThread(FRHICommandListImmediate& RHICmdList, uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtBoard, const FLinearColor DebugColor) = 0;

		// It Might need to be on rendering thread, render QUEUE is required
		virtual std::unique_ptr<rive::pls::PLSRenderer> GetPLSRenderer(const FLinearColor DebugColor) const = 0;
		
	protected:
		mutable FCriticalSection ThreadDataCS;

		TSharedPtr<Private::FRiveRenderer> RiveRenderer;
		
		FName RiveName;
		
		TObjectPtr<UTextureRenderTarget2D> RenderTarget;
	};
}