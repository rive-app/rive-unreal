// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"

#if WITH_RIVE

class URiveAsset;
class URiveTextureObject;

namespace rive
{
class Asset;
}

THIRD_PARTY_INCLUDES_START
#include "rive/file_asset_loader.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

/**
 * Unreal extension of rive::FileAssetLoader implementation (partial) for the
 * Unreal RHI. This loads assets (either embedded or OOB by using their loaded
 * bytes)
 */
class RIVE_API FRiveFileAssetLoader
#if WITH_RIVE
    final : public rive::FileAssetLoader
#endif // WITH_RIVE
{
    /**
     * Structor(s)
     */

public:
    FRiveFileAssetLoader(UObject* InOuter,
                         TMap<uint32, TObjectPtr<URiveAsset>>& InAssets);

#if WITH_RIVE

    //~ BEGIN : rive::FileAssetLoader Interface

public:
    virtual bool loadContents(rive::FileAsset& InAsset,
                              rive::Span<const uint8> InBandBytes,
                              rive::Factory* InFactory) override;

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
