// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RiveRenderer.h"

#if PLATFORM_WINDOWS

#if WITH_RIVE



namespace rive::pls
{
	class PLSRenderTargetD3D;
	class PLSRenderContextD3DImpl;
}

#endif // WITH_RIVE

namespace UE::Rive::Renderer::Private
{
	class FRiveRendererD3D11GPUAdaptor;

	class RIVERENDERER_API FRiveRendererD3D11 : public FRiveRenderer
	{
		class FRiveRendererD3D11GPUAdaptor;
		
		/**
		 * Structor(s)
		 */

	public:

		FRiveRendererD3D11();

		~FRiveRendererD3D11() = default;

		//~ BEGIN : IRiveRenderer Interface

	public:

		virtual IRiveRenderTargetPtr CreateTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget) override;

		virtual void CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList) override;

		virtual void CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList) override;

		//~ END : IRiveRenderer Interface

		void ResetBlendState() const;

	private:
		TUniquePtr<UE::Rive::Renderer::Private::FRiveRendererD3D11GPUAdaptor> D3D11GPUAdaptor;
	};
}

#endif // PLATFORM_WINDOWS