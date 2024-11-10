// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RiveAsset.h"
#include "RiveFontAsset.generated.h"

class UFontFace;
/**
 *
 */
UCLASS()
class RIVE_API URiveFontAsset : public URiveAsset
{
    GENERATED_BODY()

    URiveFontAsset();

    UFUNCTION(BlueprintCallable, Category = Rive)
    void LoadFontFace(UFontFace* InFont);

    UFUNCTION(BlueprintCallable, Category = Rive)
    void LoadFontBytes(const TArray<uint8>& InBytes);

    virtual bool LoadNativeAssetBytes(
        rive::FileAsset& InAsset,
        rive::Factory* InRiveFactory,
        const rive::Span<const uint8>& AssetBytes) override;
};
