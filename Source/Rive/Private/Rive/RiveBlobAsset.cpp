// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "Rive/RiveBlobAsset.h"

#include "IRiveRendererModule.h"
#include "RiveCommandBuilder.h"
#include "RiveRenderer.h"

void URiveBlobAsset::BeginDestroy()
{
    if (!IsRunningCommandlet() && !HasAnyFlags(RF_ClassDefaultObject) &&
        NativeBlobHandle != RIVE_NULL_HANDLE)
    {
        auto Renderer = IRiveRendererModule::Get().GetRenderer();
        check(Renderer);
        auto& CommandBuilder = Renderer->GetCommandBuilder();
        CommandBuilder.DestroyBlob(NativeBlobHandle);
    }

    Super::BeginDestroy();
}

URiveBlobAsset* URiveBlobAsset::MakeBlobAsset(const TArray<uint8>& Bytes)
{
    auto Renderer = IRiveRendererModule::Get().GetRenderer();
    check(Renderer);
    auto* BlobAsset = NewObject<URiveBlobAsset>();
    BlobAsset->Initialize(Renderer->GetCommandBuilder(), Bytes);
    return BlobAsset;
}

void URiveBlobAsset::Initialize(FRiveCommandBuilder& Builder,
                                const TArray<uint8>& Bytes)
{
    NativeBlobHandle = Builder.CreateBlobAsset(Bytes);
}
