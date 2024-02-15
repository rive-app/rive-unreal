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
	class FRiveRendererD3D11;

	class FRiveRenderTargetD3D11 final : public FRiveRenderTarget
	{
		/**
		 * Structor(s)
		 */

	public:

		FRiveRenderTargetD3D11(const TSharedRef<FRiveRendererD3D11>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget);
		virtual ~FRiveRenderTargetD3D11() override;
		//~ BEGIN : IRiveRenderTarget Interface

	public:

		virtual void Initialize() override;

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override;

#if WITH_RIVE
		
		//~ END : IRiveRenderTarget Interface

		//~ BEGIN : FRiveRenderTarget Interface

	protected:
		// It Might need to be on rendering thread, render QUEUE is required
		virtual std::unique_ptr<rive::pls::PLSRenderer> BeginFrame() const override;
		virtual void EndFrame() const override;

		//~ END : FRiveRenderTarget Interface

		/**
		 * Attribute(s)
		 */

	private:
		void ResetBlendState() const;

	private:
		TSharedRef<FRiveRendererD3D11> RiveRendererD3D11;

		rive::rcp<rive::pls::PLSRenderTargetD3D> CachedPLSRenderTargetD3D;

#endif // WITH_RIVE

		mutable bool bIsCleared = false;
	};
}

#endif // PLATFORM_WINDOWS