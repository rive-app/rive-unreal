// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RiveAsset.h"
#include "RiveAudioAsset.generated.h"

/**
 *
 */
UCLASS()
class RIVE_API URiveAudioAsset : public URiveAsset
{
    GENERATED_BODY()

    URiveAudioAsset();
    virtual bool LoadNativeAssetBytes(
        rive::FileAsset& InAsset,
        rive::Factory* InRiveFactory,
        const rive::Span<const uint8>& AssetBytes) override;
};
