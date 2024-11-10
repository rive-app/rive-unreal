// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#if WITH_RIVE

THIRD_PARTY_INCLUDES_START
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE
#include "RiveAsset.generated.h"

UENUM(BlueprintType)
enum class ERiveAssetType : uint8
{
    None = 0,
    FileBase,
    Image,
    Font,
    Audio
};

/**
 *
 */
UCLASS(BlueprintType)
class RIVE_API URiveAsset : public UObject
{
    GENERATED_BODY()

public:
    virtual void PostLoad() override;
    virtual bool LoadNativeAssetBytes(rive::FileAsset& InAsset,
                                      rive::Factory* InRiveFactory,
                                      const rive::Span<const uint8>& AssetBytes)
    {
        return false;
    }

    UPROPERTY(VisibleAnywhere, Category = Rive, meta = (NoResetToDefault))
    uint32 Id;

    UPROPERTY(VisibleAnywhere, Category = Rive, meta = (NoResetToDefault))
    ERiveAssetType Type;

    UPROPERTY(VisibleAnywhere, Category = Rive, meta = (NoResetToDefault))
    FString Name;

    UPROPERTY(VisibleAnywhere, Category = Rive, meta = (NoResetToDefault))
    bool bIsInBand;

    UPROPERTY()
    uint64 ByteSize;

    UPROPERTY()
    FString AssetPath;

    UPROPERTY()
    TArray<uint8> NativeAssetBytes;

    rive::Asset* NativeAsset;
};
