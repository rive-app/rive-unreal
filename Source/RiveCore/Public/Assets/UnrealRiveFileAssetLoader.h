// Copyright Rive, Inc. All rights reserved.

#pragma once

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/file_asset_loader.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Assets
{
    class FUnrealRiveFileAssetLoader;

    /**
     * Type definition for unique pointer reference to the instance of FUnrealRiveFileAssetLoader.
     */
    using FUnrealRiveFileAssetLoaderPtr = TUniquePtr<FUnrealRiveFileAssetLoader>;

    struct FAssetRef
    {
        uint16 Type;

        FString Name;

        void* Asset;
    };

    /**
     * Unreal extension of rive::FileAssetLoader implementation (partial) for the Unreal RHI.
     */
    class RIVECORE_API FUnrealRiveFileAssetLoader
#if WITH_RIVE
        final : public rive::FileAssetLoader
#endif // WITH_RIVE
    {
        /**
         * Structor(s)
         */

    public:

        FUnrealRiveFileAssetLoader(const FString& InRiveFilePath);

#if WITH_RIVE

        //~ BEGIN : rive::FileAssetLoader Interface

    public:

        bool loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes, rive::Factory* InFactory) override;

        //~ END : rive::FileAssetLoader Interface

#endif // WITH_RIVE

        /**
         * Attribute(s)
         */

    private:

        TMap<uint32, FAssetRef> Assets;

        FString RelativePath;
    };
}
