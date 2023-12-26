// Copyright Rive, Inc. All rights reserved.

#include "Core/UnrealRiveFactory.h"
#include "Core/UnrealRiveGradient.h"
#include "Core/UnrealRivePaint.h"
#include "Core/UnrealRivePath.h"
#include "Core/UnrealRiveTexture.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/decoders/bitmap_decoder.hpp"
#ifdef PI
#undef PI
#include "rive/math/math_types.hpp"
#endif // PI
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE::Rive::Core::FUnrealRiveFactory::FUnrealRiveFactory()
{
}

UE::Rive::Core::FUnrealRiveFactory::~FUnrealRiveFactory()
{
}

#if WITH_RIVE

rive::rcp<rive::RenderBuffer> UE::Rive::Core::FUnrealRiveFactory::makeRenderBuffer(rive::RenderBufferType InType, rive::RenderBufferFlags InFlags, size_t SizeInBytes)
{
    return rive::rcp<rive::RenderBuffer>();
}

rive::rcp<rive::RenderShader> UE::Rive::Core::FUnrealRiveFactory::makeLinearGradient(float BeginsFromX, float BeginsFromY, float EndsAtX, float EndsAtY, const rive::ColorInt InColors[], const float InStops[], size_t InCount)
{
    return Private::FUnrealRiveGradient::MakeLinear(BeginsFromX, BeginsFromY, EndsAtX, EndsAtY, InColors, InStops, InCount);
}

rive::rcp<rive::RenderShader> UE::Rive::Core::FUnrealRiveFactory::makeRadialGradient(float CenterX, float CenterY, float InRadius, const rive::ColorInt InColors[], const float InStops[], size_t InCount)
{
    return Private::FUnrealRiveGradient::MakeRadial(CenterX, CenterY, InRadius, InColors, InStops, InCount);
}

std::unique_ptr<rive::RenderPath> UE::Rive::Core::FUnrealRiveFactory::makeRenderPath(rive::RawPath& InRawPath, rive::FillRule InFillMode)
{
    return std::make_unique<Private::FUnrealRivePath>(InFillMode, InRawPath);
}

std::unique_ptr<rive::RenderPath> UE::Rive::Core::FUnrealRiveFactory::makeEmptyRenderPath()
{
    return std::make_unique<Private::FUnrealRivePath>();
}

std::unique_ptr<rive::RenderPaint> UE::Rive::Core::FUnrealRiveFactory::makeRenderPaint()
{
    return std::make_unique<Private::FUnrealRivePaint>();
}

rive::rcp<rive::RenderImage> UE::Rive::Core::FUnrealRiveFactory::decodeImage(rive::Span<const uint8_t> InEncodedBytes)
{
    std::unique_ptr<Bitmap> NewBitmap = Bitmap::decode(InEncodedBytes.data(), InEncodedBytes.size());
    
    if (NewBitmap)
    {
        // For now, Rive only accepts RGBA.
        if (NewBitmap->pixelFormat() != Bitmap::PixelFormat::RGBA)
        {
            NewBitmap->pixelFormat(Bitmap::PixelFormat::RGBA);
        }
        
        const uint32 Width = NewBitmap->width();
        
        const uint32 Height = NewBitmap->height();
        
        const uint32 MipLevelCount = rive::math::msb(Height | Width);
        
        FRHITextureCreateDesc CreateDesc = FRHITextureCreateDesc::Create2D(TEXT("Rive.Image"), Width, Height, PF_R8G8B8A8);

        CreateDesc.SetNumMips(MipLevelCount);

        return rive::rcp<rive::RenderImage>(new Private::FUnrealRiveImage(CreateDesc, (uint8*)NewBitmap->bytes(), NewBitmap->byteSize()));
    }

    return rive::rcp<rive::RenderImage>();
}

#endif // WITH_RIVE
