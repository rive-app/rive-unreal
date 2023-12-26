// Copyright Rive, Inc. All rights reserved.

#include "Core/UnrealRivePaint.h"
#include "Core/UnrealRiveGradient.h"
#include "Core/UnrealRiveTexture.h"

#if WITH_RIVE

void UE::Rive::Core::Private::FUnrealRivePaint::style(rive::RenderPaintStyle NewStyle)
{
    bIsStroked = NewStyle == rive::RenderPaintStyle::stroke;
}

void UE::Rive::Core::Private::FUnrealRivePaint::color(rive::ColorInt color)
{
    ImageTexture.reset();

    Gradient.reset();

    Color = color;

    PaintType = EPaintType::SolidColor;
}

void UE::Rive::Core::Private::FUnrealRivePaint::thickness(float NewThickness)
{
    Thickness = FMath::Abs(NewThickness);
}

void UE::Rive::Core::Private::FUnrealRivePaint::join(rive::StrokeJoin NewJoinType)
{
    JoinType = NewJoinType;
}

void UE::Rive::Core::Private::FUnrealRivePaint::cap(rive::StrokeCap NewCapType)
{
    CapType = NewCapType;
}

void UE::Rive::Core::Private::FUnrealRivePaint::blendMode(rive::BlendMode NewBlendMode)
{
    BlendMode = NewBlendMode;
}

void UE::Rive::Core::Private::FUnrealRivePaint::shader(rive::rcp<rive::RenderShader> NewShader)
{
    ImageTexture.reset();

    Gradient = rive::static_rcp_cast<FUnrealRiveGradient>(MoveTemp(NewShader));

    PaintType = Gradient ? Gradient->GetPaintType() : EPaintType::SolidColor;
}

void UE::Rive::Core::Private::FUnrealRivePaint::image(rive::rcp<const FUnrealRiveTexture> NewImageTexture, float InOpacity)
{
    ImageOpacity = InOpacity;

    ImageTexture = MoveTemp(NewImageTexture);

    Gradient.reset();

    PaintType = EPaintType::Image;
}

void UE::Rive::Core::Private::FUnrealRivePaint::invalidateStroke()
{
}

UE::Rive::Core::EPaintType UE::Rive::Core::Private::FUnrealRivePaint::GetType() const
{
    return PaintType;
}

bool UE::Rive::Core::Private::FUnrealRivePaint::IsStroked() const
{
    return bIsStroked;
}

rive::ColorInt UE::Rive::Core::Private::FUnrealRivePaint::GetColor() const
{
    return Color;
}

float UE::Rive::Core::Private::FUnrealRivePaint::GetThickness() const
{
    return Thickness;
}

const UE::Rive::Core::Private::FUnrealRiveGradient* UE::Rive::Core::Private::FUnrealRivePaint::GetGradient() const
{
    return Gradient.get();
}

const UE::Rive::Core::Private::FUnrealRiveTexture* UE::Rive::Core::Private::FUnrealRivePaint::GetImageTexture() const
{
    return ImageTexture.get();
}

float UE::Rive::Core::Private::FUnrealRivePaint::GetImageOpacity() const
{
    return ImageOpacity;
}

rive::StrokeJoin UE::Rive::Core::Private::FUnrealRivePaint::GetJoinType() const
{
    return JoinType;
}

rive::StrokeCap UE::Rive::Core::Private::FUnrealRivePaint::GetCapType() const
{
    return CapType;
}

rive::BlendMode UE::Rive::Core::Private::FUnrealRivePaint::GetBlendMode() const
{
    return BlendMode;
}

#endif // WITH_RIVE
