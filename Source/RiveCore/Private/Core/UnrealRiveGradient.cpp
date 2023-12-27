// Copyright Rive, Inc. All rights reserved.

#include "Core/UnrealRiveGradient.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/math/simd.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE::Rive::Core::Private::FUnrealRiveGradient::FUnrealRiveGradient()
    : PaintType(EPaintType::None)
    , Colors()
    , Stops()
    , Count(0)
    , Coeffs()
{
}

UE::Rive::Core::Private::FUnrealRiveGradient::~FUnrealRiveGradient()
{
}

#if WITH_RIVE

UE::Rive::Core::Private::FUnrealRiveGradient::FUnrealRiveGradient(EPaintType InPaintType, FUnrealRiveGradData<rive::ColorInt>&& InColors, FUnrealRiveGradData<float>&& InStops, size_t InCount, float CoeffX, float CoeffY, float CoeffZ)
    : PaintType(InPaintType)
    , Colors(MoveTemp(InColors))
    , Stops(MoveTemp(InStops))
    , Count(InCount)
    , Coeffs()
{
    Coeffs[0] = CoeffX;

    Coeffs[1] = CoeffY;

    Coeffs[2] = CoeffZ;

    check(InPaintType == EPaintType::LinearGradient || InPaintType == EPaintType::RadialGradient);
}

rive::rcp<UE::Rive::Core::Private::FUnrealRiveGradient> UE::Rive::Core::Private::FUnrealRiveGradient::MakeLinear(float BeginsFromX, float BeginsFromY, float EndsAtX, float EndsAtY, const rive::ColorInt InColors[], const float InStops[], size_t InCount)
{
    rive::float2 BeginsFrom = { BeginsFromX, BeginsFromY };

    rive::float2 EndsAt = { EndsAtX, EndsAtY };

    FUnrealRiveGradData<rive::ColorInt> NewColors(InColors, InCount);

    FUnrealRiveGradData<float> NewStops(InStops, InCount);

    // If the stops don't begin and end on 0 and 1, transform the gradient so they do. This allows
    // us to take full advantage of the gradient's range of pixels in the texture.
    float FirstStop = InStops[0];

    float LastStop = InStops[InCount - 1];

    if (FirstStop != 0 || LastStop != 1)
    {
        // Tighten the endpoints to align with the mininum and maximum gradient stops.
        rive::float4 NewEndpoints = rive::simd::precise_mix(BeginsFrom.xyxy,
            EndsAt.xyxy,
            rive::float4{ FirstStop, FirstStop, LastStop, LastStop });

        BeginsFrom = NewEndpoints.xy;

        EndsAt = NewEndpoints.zw;

        // Transform the stops into the range defined by the new endpoints.
        NewStops[0] = 0;

        if (InCount > 2)
        {
            const float Mid = 1.f / (LastStop - FirstStop);

            const float LowBound = -FirstStop * Mid;

            for (size_t StopIndex = 1; StopIndex < InCount - 1; ++StopIndex)
            {
                NewStops[StopIndex] = FMath::Clamp(InStops[StopIndex] * Mid + LowBound, NewStops[StopIndex - 1], 1.f);
            }
        }

        NewStops[InCount - 1] = 1;
    }

    rive::float2 Range = EndsAt - BeginsFrom;

    Range *= 1.f / rive::simd::dot(Range, Range); // dot(Range, EndsAt - BeginsFrom) == 1

    return rive::rcp(new FUnrealRiveGradient(EPaintType::LinearGradient,
        MoveTemp(NewColors),
        MoveTemp(NewStops),
        InCount,
        Range.x,
        Range.y,
        -rive::simd::dot(Range, BeginsFrom)));
}

rive::rcp<UE::Rive::Core::Private::FUnrealRiveGradient> UE::Rive::Core::Private::FUnrealRiveGradient::MakeRadial(float CenterX, float CenterY, float InRadius, const rive::ColorInt InColors[], const float InStops[], size_t InCount)
{
    FUnrealRiveGradData<rive::ColorInt> NewColors(InColors, InCount);

    FUnrealRiveGradData<float> NewStops(InStops, InCount);

    // If the stops don't end on 1, scale the gradient so they do. This allows us to take better
    // advantage of the gradient's full range of pixels in the texture.
    //
    // TODO: If we want to take full advantage of the gradient texture pixels, we could add an inner
    // radius that specifies where t=0 begins (instead of assuming it begins at the center).
    const float LastStop = InStops[InCount - 1];

    if (LastStop != 1)
    {
        // Scale the radius to align with the final stop.
        InRadius *= LastStop;

        // Scale the stops into the range defined by the new radius.
        float InverseLastStop = 1.f / LastStop;

        for (size_t StopIndex = 0; StopIndex < InCount - 1; ++StopIndex)
        {
            NewStops[StopIndex] = InStops[StopIndex] * InverseLastStop;
        }

        NewStops[InCount - 1] = 1;
    }

    return rive::rcp(new FUnrealRiveGradient(EPaintType::RadialGradient,
        MoveTemp(NewColors),
        MoveTemp(NewStops),
        InCount,
        CenterX,
        CenterY,
        InRadius));
}

UE::Rive::Core::EPaintType UE::Rive::Core::Private::FUnrealRiveGradient::GetPaintType() const
{
    return PaintType;
}

const float* UE::Rive::Core::Private::FUnrealRiveGradient::GetCoeffs() const
{
    return Coeffs.GetData();
}

const rive::ColorInt* UE::Rive::Core::Private::FUnrealRiveGradient::GetColors() const
{
    return Colors.Get();
}

const float* UE::Rive::Core::Private::FUnrealRiveGradient::GetStops() const
{
    return Stops.Get();
}

int UE::Rive::Core::Private::FUnrealRiveGradient::GetCount() const
{
    return Count;
}

#endif // WITH_RIVE
