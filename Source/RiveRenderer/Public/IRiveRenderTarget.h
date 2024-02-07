// Copyright Rive, Inc. All rights reserved.

#pragma once

#if WITH_RIVE

THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive
{
	class Artboard;
	class Renderer;

	namespace pls
	{
		class PLSRenderContext;
	}
}

#endif // WITH_RIVE

namespace UE::Rive::Renderer
{
	class IRiveRenderTarget;

	/**
	 * Type definition for shared pointer reference to the instance of IRiveRenderTarget.
	 */
	using IRiveRenderTargetPtr = TSharedPtr<IRiveRenderTarget>;

	/**
	 * Type definition for shared pointer reference to the instance of IRiveRenderTarget.
	 */
	using IRiveRenderTargetRef = TSharedRef<IRiveRenderTarget>;

	class IRiveRenderTarget : public TSharedFromThis<IRiveRenderTarget>
	{
		/**
		 * Structor(s)
		 */

	public:

		virtual ~IRiveRenderTarget() = default;

		/**
		 * Implementation(s)
		 */

	public:

		virtual void Initialize() = 0;

#if WITH_RIVE

		virtual void DrawArtboard(uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor) = 0;

#endif // WITH_RIVE

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) = 0;
		
		virtual uint32 GetWidth() const = 0;
		
		virtual uint32 GetHeight() const = 0;

		virtual FCriticalSection& GetThreadDataCS() = 0;
	};
}
