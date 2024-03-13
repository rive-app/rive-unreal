// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "RHICommandList.h"

#if WITH_RIVE

enum class ERiveFitType : uint8;

#include "RiveCore/Public/PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#undef  PI // redefined in rive/math/math_types.hpp
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

		virtual void Submit() = 0;
		virtual void SubmitAndClear() = 0;
		virtual void Save() = 0;
		virtual void Restore() = 0;
		virtual void Transform(float X1, float Y1, float X2, float Y2, float TX, float TY) = 0;
		virtual void Translate(const FVector2f& InVector) = 0;
		virtual void Draw(rive::Artboard* InArtboard) = 0;
		virtual void Align(const FBox2f& InBox, ERiveFitType InFit, const FVector2f& InAlignment, rive::Artboard* InArtboard) = 0;
		virtual void Align(ERiveFitType InFit, const FVector2f& InAlignment, rive::Artboard* InArtboard) = 0;
		/** Returns the transformation Matrix from the start of the Render Queue up to now */
		virtual FMatrix GetTransformMatrix() const = 0;

#endif // WITH_RIVE

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) = 0;

		virtual void SetClearColor(const FLinearColor& InColor) = 0;
		virtual uint32 GetWidth() const = 0;
		virtual uint32 GetHeight() const = 0;
	};
}
