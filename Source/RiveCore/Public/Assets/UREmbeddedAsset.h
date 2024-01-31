// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "UREmbeddedAsset.generated.h"

#if WITH_RIVE
namespace rive
{
	class Asset;
}
#endif // WITH_RIVE

USTRUCT(BlueprintType)
struct FUREmbeddedAsset
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	uint16 Type;

	UPROPERTY()
	uint32 Id;

	UPROPERTY(VisibleAnywhere)
	FString Name;

	UPROPERTY(VisibleAnywhere)
	bool bIsInBand;

	UPROPERTY()
	uint64 ByteSize;

	UPROPERTY()
	TArray<uint8> Bytes;

	UPROPERTY()
	FString AssetPath;

#if WITH_RIVE
	rive::Asset* Asset;
#endif // WITH_RIVE
};
