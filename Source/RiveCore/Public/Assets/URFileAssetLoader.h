// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"

#if WITH_RIVE

class URiveAsset;
class URiveFile;

namespace rive
{
    class Asset;
}

#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/file_asset_loader.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Assets
{
    /**
     * Unreal extension of rive::FileAssetLoader implementation (partial) for the Unreal RHI.
     * This loads assets (either embedded or OOB by using their loaded bytes)
     */
    class RIVECORE_API FURFileAssetLoader
#if WITH_RIVE
        final : public rive::FileAssetLoader
#endif // WITH_RIVE
    {
        /**
         * Structor(s)
         */

    public:

        FURFileAssetLoader(UObject* InOuter, TMap<uint32, TObjectPtr<URiveAsset>>& InAssets);

#if WITH_RIVE

        //~ BEGIN : rive::FileAssetLoader Interface

    public:
        virtual bool loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes, rive::Factory* InFactory) override;

        //~ END : rive::FileAssetLoader Interface

#endif // WITH_RIVE

    public:

        /**
         * Attribute(s)
         */
    
    private:
        TObjectPtr<UObject> Outer;
        TMap<uint32, TObjectPtr<URiveAsset>>& Assets;
    };
}
