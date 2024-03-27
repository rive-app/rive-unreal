// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_ANDROID
#include "RiveRenderTarget.h"

#if WITH_RIVE
#include "RiveCore/Public/PreRiveHeaders.h"
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
		FRiveRenderTargetOpenGL(const TSharedRef<FRiveRendererOpenGL>& InRiveRenderer, const FName& InRiveName, UTexture2DDynamic* InRenderTarget);
		virtual ~FRiveRenderTargetOpenGL() override;
		//~ BEGIN : IRiveRenderTarget Interface
	public:
		virtual void Initialize() override;
		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override;

#if WITH_RIVE
		//~ END : IRiveRenderTarget Interface

		//~ BEGIN : FRiveRenderTarget Interface
		virtual void Submit() override;

	protected:
		// It Might need to be on rendering thread, render QUEUE is required
		virtual rive::rcp<rive::pls::PLSRenderTarget> GetRenderTarget() const override;
		virtual std::unique_ptr<rive::pls::PLSRenderer> BeginFrame() override;
		virtual void EndFrame() const override;
		virtual void Render_RenderThread(FRHICommandListImmediate& RHICmdList, const TArray<FRiveRenderCommand>& RiveRenderCommands) override;
		//~ END : FRiveRenderTarget Interface
		
	private:
		void CacheTextureTarget_Internal(const FTexture2DRHIRef& InRHIResource);
		
		/**
		 * Attribute(s)
		 */
		
		rive::rcp<rive::pls::TextureRenderTargetGL> CachedPLSRenderTargetOpenGL;
		TSharedPtr<FRiveRendererOpenGL> RiveRendererGL;

#endif // WITH_RIVE
	};
}
#endif