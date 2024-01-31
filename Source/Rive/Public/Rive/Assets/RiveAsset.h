// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif WITH_RIVE
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
class RIVE_API URiveAsset : public UObject
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

	UPROPERTY(VisibleAnywhere)
	uint32 Id;
	
	UPROPERTY(VisibleAnywhere)
	ERiveAssetType Type;
	
	UPROPERTY(VisibleAnywhere)
	FString Name;

	UPROPERTY(VisibleAnywhere)
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
