// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/command_queue.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveBlobAsset.generated.h"

struct FRiveCommandBuilder;

UCLASS(BlueprintType)
class RIVE_API URiveBlobAsset : public UObject
{
    GENERATED_BODY()

public:
    virtual void BeginDestroy() override;

    UFUNCTION(BlueprintCallable, Category = Rive)
    static URiveBlobAsset* MakeBlobAsset(const TArray<uint8>& Bytes);

    void Initialize(FRiveCommandBuilder& Builder, const TArray<uint8>& Bytes);

    rive::BlobAssetHandle GetNativeBlobHandle() const
    {
        return NativeBlobHandle;
    }

private:
    rive::BlobAssetHandle NativeBlobHandle = RIVE_NULL_HANDLE;
};
