// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/TextureThumbnailRenderer.h"
#include "RiveFileThumbnailRenderer.generated.h"

class IRiveRenderTarget;
class URiveArtboard;
class URiveTexture;

/**
 *
 */
UCLASS()
class RIVEEDITOR_API URiveFileThumbnailRenderer
    : public UTextureThumbnailRenderer
{
    GENERATED_BODY()

    URiveFileThumbnailRenderer();

    virtual bool CanVisualizeAsset(UObject* Object) override;
    virtual EThumbnailRenderFrequency GetThumbnailRenderFrequency(
        UObject* Object) const override;
    virtual void Draw(UObject* Object,
                      int32 X,
                      int32 Y,
                      uint32 Width,
                      uint32 Height,
                      FRenderTarget* Viewport,
                      FCanvas* Canvas,
                      bool bAdditionalViewFamily) override;

private:
    void OnAssetRemoved(const FAssetData& AssetData);

    UPROPERTY()
    TMap<FName, URiveArtboard*> ThumbnailRenderers;

    UPROPERTY()
    URiveTexture* RiveTexture;

    TSharedPtr<IRiveRenderTarget> RiveRenderTarget;
    bool Initialized = false;
};
