// Copyright Rive, Inc. All rights reserved.

#pragma once

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/factory.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Core
{
    class FUnrealRiveFactory;

    /**
     * Type definition for unique pointer reference to the instance of FUnrealRiveFactory.
     */
    using FUnrealRiveFactoryPtr = TUniquePtr<FUnrealRiveFactory>;

    /**
     * Unreal extension of rive::Factory implementation (partial) for the Unreal RHI.
     */
    class RIVECORE_API  FUnrealRiveFactory
#if WITH_RIVE
        final : public rive::Factory
#endif // WITH_RIVE
    {
        /**
         * Structor(s)
         */

    public:

        FUnrealRiveFactory();

        ~FUnrealRiveFactory();

#if WITH_RIVE

        //~ BEGIN : rive::Factory Interface

    public:

        rive::rcp<rive::RenderBuffer> makeRenderBuffer(rive::RenderBufferType InType, rive::RenderBufferFlags InFlags, size_t SizeInBytes) override;

        rive::rcp<rive::RenderShader> makeLinearGradient(float BeginsFromX,
            float BeginsFromY,
            float EndsAtX,
            float EndsAtY,
            const rive::ColorInt InColors[], // [count]
            const float InStops[],     // [count]
            size_t InCount) override;

        rive::rcp<rive::RenderShader> makeRadialGradient(float CenterX,
            float CenterY,
            float InRadius,
            const rive::ColorInt InColors[], // [count]
            const float InStops[],     // [count]
            size_t InCount) override;

        // Returns a full-formed RenderPath -- can be treated as immutable
        std::unique_ptr<rive::RenderPath> makeRenderPath(rive::RawPath& InRawPath, rive::FillRule InFillMode) override;

        std::unique_ptr<rive::RenderPath> makeEmptyRenderPath() override;

        std::unique_ptr<rive::RenderPaint> makeRenderPaint() override;

        rive::rcp<rive::RenderImage> decodeImage(rive::Span<const uint8_t> InEncodedBytes) override;

        //~ END : rive::Factory Interface

#endif // WITH_RIVE
    };
}