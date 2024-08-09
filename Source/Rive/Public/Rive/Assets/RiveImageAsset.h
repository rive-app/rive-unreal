// Fill out your copyright notice in the Description page of Project Settings.

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

	UFUNCTION(BlueprintCallable)
	void LoadTexture(UTexture2D* InTexture);

	virtual bool LoadNativeAssetBytes(rive::FileAsset& InAsset, rive::Factory* InRiveFactory, const rive::Span<const uint8>& AssetBytes) override;
};
