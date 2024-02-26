// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#if WITH_RIVE
#include "PreRiveHeaders.h"
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
	FileBase = 103,
	Image = 105,
	Font = 141
};

/**
 * 
 */
UCLASS(BlueprintType)
class RIVECORE_API URiveAsset : public UObject
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

	UPROPERTY(VisibleAnywhere, Category=Rive)
	uint32 Id;
	
	UPROPERTY(VisibleAnywhere, Category=Rive)
	ERiveAssetType Type;
	
	UPROPERTY(VisibleAnywhere, Category=Rive)
	FString Name;

	UPROPERTY(VisibleAnywhere, Category=Rive)
	bool bIsInBand;

	UPROPERTY()
	uint64 ByteSize;

	UPROPERTY()
	FString AssetPath;
	
	UPROPERTY()
	TArray<uint8> NativeAssetBytes;

#if WITH_RIVE
	rive::Asset* NativeAsset;
#else
	void* NativeAsset;
#endif

	void LoadFromDisk();
	bool DecodeNativeAsset(rive::FileAsset& InAsset, rive::Factory* InRiveFactory, const rive::Span<const uint8>& AssetBytes);
private:
	bool DecodeImageAsset(rive::FileAsset& InAsset, rive::Factory* InRiveFactory, const rive::Span<const uint8>& AssetBytes);
	bool DecodeFontAsset(rive::FileAsset& InAsset, rive::Factory* InRiveFactory, const rive::Span<const uint8>& AssetBytes);
};
