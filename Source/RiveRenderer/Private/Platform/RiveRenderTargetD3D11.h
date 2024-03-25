// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RiveRenderTarget.h"

#if PLATFORM_WINDOWS

#if WITH_RIVE
#include "RiveCore/Public/PreRiveHeaders.h"
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
	public:
		/**
		 * Structor(s)
		 */

		FRiveRenderTargetD3D11(const TSharedRef<FRiveRendererD3D11>& InRiveRenderer, const FName& InRiveName, UTexture2DDynamic* InRenderTarget);
		virtual ~FRiveRenderTargetD3D11() override;
		
		//~ BEGIN : IRiveRenderTarget Interface
	public:
		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override;
		//~ END : IRiveRenderTarget Interface
		
#if WITH_RIVE
		//~ BEGIN : FRiveRenderTarget Interface
	protected:
		// It Might need to be on rendering thread, render QUEUE is required
		virtual void Render_RenderThread(FRHICommandListImmediate& RHICmdList, const TArray<FRiveRenderCommand>& RiveRenderCommands) override;
		virtual rive::rcp<rive::pls::PLSRenderTarget> GetRenderTarget() const override;
		//~ END : FRiveRenderTarget Interface

		/**
		 * Attribute(s)
		 */
	private:
		TSharedRef<FRiveRendererD3D11> RiveRendererD3D11;
		rive::rcp<rive::pls::PLSRenderTargetD3D> CachedPLSRenderTargetD3D;
#endif // WITH_RIVE
	};
}

#endif // PLATFORM_WINDOWS