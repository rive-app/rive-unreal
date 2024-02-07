// Copyright Rive, Inc. All rights reserved.

#pragma once

#if WITH_RIVE

class URiveFile;

namespace rive
{
    class Asset;
}

THIRD_PARTY_INCLUDES_START
#include "rive/file_asset_loader.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Assets
{
    class FURFileAssetLoader;

    /**
     * Type definition for unique pointer reference to the instance of FURFileAssetLoader.
     */
    using FURFileAssetLoaderPtr = TUniquePtr<FURFileAssetLoader>;

    /**
     * Unreal extension of rive::FileAssetLoader implementation (partial) for the Unreal RHI.
     * This loads assets (either embedded or OOB by using their loaded bytes)
     */
    class RIVE_API FURFileAssetLoader
#if WITH_RIVE
        final : public rive::FileAssetLoader
#endif // WITH_RIVE
    {
        /**
         * Structor(s)
         */

    public:

        FURFileAssetLoader(URiveFile* InRiveFile);

#if WITH_RIVE

        //~ BEGIN : rive::FileAssetLoader Interface

    public:

        bool loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes, rive::Factory* InFactory) override;

        //~ END : rive::FileAssetLoader Interface

#endif // WITH_RIVE

    public:

        /**
         * Attribute(s)
         */
    
    private:
        URiveFile* RiveFile;
    };
}
