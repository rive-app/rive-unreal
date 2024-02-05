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
	uint16 Type = 0;

	UPROPERTY()
	uint32 Id = 0;

	UPROPERTY(VisibleAnywhere)
	FString Name;

	UPROPERTY(VisibleAnywhere)
	bool bIsInBand = false;

	UPROPERTY()
	uint64 ByteSize = 0;

	UPROPERTY()
	TArray<uint8> Bytes;

	UPROPERTY()
	FString AssetPath;

#if WITH_RIVE
	rive::Asset* Asset;
#endif // WITH_RIVE
};
