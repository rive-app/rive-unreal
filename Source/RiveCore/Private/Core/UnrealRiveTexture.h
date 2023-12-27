// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Core/UnrealRiveCoreTypes.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/refcnt.hpp"
#include "rive/renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Core::Private
{
    /**
     * Unreal extension of Rive's texture local storage renderer.
     */
    class FUnrealRiveTexture
#if WITH_RIVE
        final : public rive::RefCnt<FUnrealRiveTexture>
#endif // WITH_RIVE
    {
        /**
         * Structor(s)
         */

    public:

        FUnrealRiveTexture(uint32 InWidth, uint32 InHeight);

        virtual ~FUnrealRiveTexture() = default;

        /**
         * Implementation(s)
         */

    public:

        uint32 GetWidth() const;

        uint32 GetHeight() const;

        /**
         * Attribute(s)
         */

    private:

        uint32 Width;

        uint32 Height;
    };

    /**
     * Unreal extension of Rive's image local storage renderer.
     */
    class FUnrealRiveImage
#if WITH_RIVE
        final : public rive::lite_rtti_override<rive::RenderImage, FUnrealRiveImage>
#endif // WITH_RIVE
    {
        /**
         * Structor(s)
         */

    public:

        FUnrealRiveImage(const FRHITextureCreateDesc& CreateDesc, uint8* InBitmap, size_t SizeInBytes);

    protected:

        FUnrealRiveImage(uint32 InWidth, uint32 InHeight);

        /**
         * Implementation(s)
         */

    public:

        FTextureRHIRef GetTextureRef() const;
        
        const FRHITexture* GetTexture() const;

    protected:

        void ResetTexture(const FRHITextureCreateDesc& CreateDesc, uint8* InBitmap = nullptr, size_t SizeInBytes = 0);

        /**
         * Attribute(s)
         */

    private:

        FTextureRHIRef Texture;
    };
}
