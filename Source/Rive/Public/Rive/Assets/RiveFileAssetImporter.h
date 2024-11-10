// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"

#if WITH_RIVE
class URiveAsset;
class URiveTextureObject;

THIRD_PARTY_INCLUDES_START
#include "rive/file_asset_loader.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

/**
 * Unreal extension of rive::FileAssetLoader implementation (partial) for the
 * Unreal RHI.
 */
class RIVE_API FRiveFileAssetImporter
#if WITH_RIVE
    final : public rive::FileAssetLoader
#endif // WITH_RIVE
{
    /**
     * Structor(s)
     */

public:
    FRiveFileAssetImporter(UPackage* InPackage,
                           const FString& InRiveFilePath,
                           TMap<uint32, TObjectPtr<URiveAsset>>& InAssets);

#if WITH_RIVE

    //~ BEGIN : rive::FileAssetLoader Interface

public:
    virtual bool loadContents(rive::FileAsset& InAsset,
                              rive::Span<const uint8> InBandBytes,
                              rive::Factory* InFactory) override;

    //~ END : rive::FileAssetLoader Interface

#endif // WITH_RIVE

    /**
     * Attribute(s)
     */

public:
    UPackage* RivePackage;
    FString RiveFilePath;
    TMap<uint32, TObjectPtr<URiveAsset>>& Assets;
};
