// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"

#if WITH_RIVE

THIRD_PARTY_INCLUDES_START
#include "rive/refcnt.hpp"
THIRD_PARTY_INCLUDES_END

struct FRiveRenderCommand;

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
	public:
		FRiveRenderTarget(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget);
		virtual ~FRiveRenderTarget() override;

		virtual void Initialize() override;
		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override {}
		virtual uint32 GetWidth() const override;
		virtual uint32 GetHeight() const override;
		virtual void SetClearColor(const FLinearColor& InColor) override { ClearColor = InColor; }
#if WITH_RIVE
		virtual void Submit() override;
		virtual void SubmitAndClear() override;
		virtual void Save() override;
		virtual void Restore() override;
		virtual void Transform(float X1, float Y1, float X2, float Y2, float TX, float TY) override;
		virtual void Draw(rive::Artboard* InArtboard) override;
		virtual void Align(ERiveFitType InFit, const FVector2f& InAlignment, rive::Artboard* InArtboard) override;
		
	protected:
		virtual rive::rcp<rive::pls::PLSRenderTarget> GetRenderTarget() const = 0;
		virtual std::unique_ptr<rive::pls::PLSRenderer> BeginFrame();
		virtual void EndFrame() const;
		virtual void Render_RenderThread(FRHICommandListImmediate& RHICmdList);
		virtual void Render_Internal();
#endif // WITH_RIVE
	
	protected:
		mutable bool bClearQueue = false;
		mutable bool bIsCleared = false;
		FLinearColor ClearColor = FLinearColor::Transparent;
		FName RiveName;
		TObjectPtr<UTextureRenderTarget2D> RenderTarget;
		TArray<FRiveRenderCommand> RenderCommands;
		TSharedPtr<FRiveRenderer> RiveRenderer;
		mutable FDateTime LastResetTime = FDateTime::Now();
		static FTimespan ResetTimeLimit;
	};
}
