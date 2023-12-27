// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Core/UnrealRiveCoreTypes.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/shapes/paint/color.hpp"
#include "rive/renderer.hpp"
#include "utils/lite_rtti.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Core::Private
{
    class FUnrealRiveGradient;
    class FUnrealRivePaint;
    class FUnrealRiveTexture;

    /**
     * Type definition for unique pointer reference to the instance of FUnrealRivePaint.
     */
    using FUnrealRivePaintPtr = std::unique_ptr<FUnrealRivePaint>;

    /**
     * Unreal extension of RenderPaint implementation for Rive's pixel local storage renderer.
     */
    class FUnrealRivePaint
#if WITH_RIVE
        final : public rive::lite_rtti_override<rive::RenderPaint, FUnrealRivePaint>
#endif // WITH_RIVE
    {

        /**
         * Structor(s)
         */

    public:

        FUnrealRivePaint() = default;

        ~FUnrealRivePaint() = default;

#if WITH_RIVE

        //~ BEGIN : rive::RenderPaint Interface

    public:

        void style(rive::RenderPaintStyle NewStyle) override;

        void color(rive::ColorInt color) override;

        void thickness(float NewThickness) override;

        void join(rive::StrokeJoin NewJoinType) override;

        void cap(rive::StrokeCap NewCapType) override;

        void blendMode(rive::BlendMode NewBlendMode) override;

        void shader(rive::rcp<rive::RenderShader> NewShader) override;

        void image(rive::rcp<const FUnrealRiveTexture> NewImageTexture, float InOpacity);

        void invalidateStroke() override;

        //~ END : rive::RenderPaint Interface

        /**
         * Implementation(s)
         */

    public:

        EPaintType GetType() const;

        bool IsStroked() const;

        rive::ColorInt GetColor() const;

        float GetThickness() const;

        const FUnrealRiveGradient* GetGradient() const;

        const FUnrealRiveTexture* GetImageTexture() const;

        float GetImageOpacity() const;

        rive::StrokeJoin GetJoinType() const;

        rive::StrokeCap GetCapType() const;

        rive::BlendMode GetBlendMode() const;

        /**
         * Attribute(s)
         */

    private:

        EPaintType PaintType = EPaintType::SolidColor;

        bool bIsStroked = false;

        rive::ColorInt Color = 0xff000000;

        rive::rcp<FUnrealRiveGradient> Gradient;

        rive::rcp<const FUnrealRiveTexture> ImageTexture;

        float ImageOpacity;

        float Thickness = 1;

        rive::StrokeJoin JoinType = rive::StrokeJoin::miter;

        rive::StrokeCap CapType = rive::StrokeCap::butt;

        rive::BlendMode BlendMode = rive::BlendMode::srcOver;

#endif // WITH_RIVE
    };
}
