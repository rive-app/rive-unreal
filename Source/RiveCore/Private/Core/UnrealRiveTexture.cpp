// Copyright Rive, Inc. All rights reserved.

#include "Core/UnrealRiveTexture.h"

UE::Rive::Core::Private::FUnrealRiveTexture::FUnrealRiveTexture(uint32 InWidth, uint32 InHeight)
    : Width(InWidth)
    , Height(InHeight)
{
}

uint32 UE::Rive::Core::Private::FUnrealRiveTexture::GetWidth() const
{
    return Width;
}

uint32 UE::Rive::Core::Private::FUnrealRiveTexture::GetHeight() const
{
    return Height;
}

UE::Rive::Core::Private::FUnrealRiveImage::FUnrealRiveImage(const FRHITextureCreateDesc& CreateDesc, uint8* InBitmap, size_t SizeInBytes)
    : FUnrealRiveImage(CreateDesc.GetSize().X, CreateDesc.GetSize().Y)
{
    ResetTexture(CreateDesc, InBitmap, SizeInBytes);
}

UE::Rive::Core::Private::FUnrealRiveImage::FUnrealRiveImage(uint32 InWidth, uint32 InHeight)
{
    m_Width = InWidth;

    m_Height = InHeight;
}

FTextureRHIRef UE::Rive::Core::Private::FUnrealRiveImage::GetTextureRef() const
{
    return Texture;
}

const FRHITexture* UE::Rive::Core::Private::FUnrealRiveImage::GetTexture() const
{
    return Texture;
}

void UE::Rive::Core::Private::FUnrealRiveImage::ResetTexture(const FRHITextureCreateDesc& CreateDesc, uint8* InBitmap, size_t SizeInBytes)
{
    check(InBitmap == nullptr || CreateDesc.GetSize().X == m_Width);

    check(InBitmap == nullptr || CreateDesc.GetSize().Y == m_Height);

    Texture = RHICreateTexture(CreateDesc);

    // Write the contents of the texture.
    uint32 DestStride;

    constexpr bool bLockWithinMiptail = false;

    constexpr uint32 MipIndex = 0;

    uint8* DestBuffer = (uint8*)RHILockTexture2D(Texture, MipIndex, RLM_WriteOnly, DestStride, bLockWithinMiptail);
    
    FMemory::Memcpy(DestBuffer, InBitmap, SizeInBytes);

    RHIUnlockTexture2D(Texture, MipIndex, bLockWithinMiptail);
}
