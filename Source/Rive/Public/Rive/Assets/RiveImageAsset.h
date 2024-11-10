// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveAsset.h"
#include "RiveImageAsset.generated.h"

/**
 *
 */
UCLASS()
class RIVE_API URiveImageAsset : public URiveAsset
{
    GENERATED_BODY()

    URiveImageAsset();

    UFUNCTION(BlueprintCallable, Category = Rive)
    void LoadTexture(UTexture2D* InTexture);

    UFUNCTION(BlueprintCallable, Category = Rive)
    void LoadImageBytes(const TArray<uint8>& InBytes);

    virtual bool LoadNativeAssetBytes(
        rive::FileAsset& InAsset,
        rive::Factory* InRiveFactory,
        const rive::Span<const uint8>& AssetBytes) override;
};
