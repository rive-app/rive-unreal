// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveAsset.h"
#include "RiveAudioAsset.generated.h"

UCLASS()
class RIVE_API URiveAudioAsset : public URiveAsset
{
    GENERATED_BODY()

public:
    URiveAudioAsset();

    virtual bool LoadNativeAssetBytes(
        rive::FileAsset& InAsset,
        rive::Factory* InRiveFactory,
        const rive::Span<const uint8>& AssetBytes) override;
};
