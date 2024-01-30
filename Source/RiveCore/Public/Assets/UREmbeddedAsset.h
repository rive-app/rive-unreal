#pragma once
#include "UREmbeddedAsset.generated.h"

namespace rive
{
	class Asset;
}

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

	rive::Asset* Asset;
};
