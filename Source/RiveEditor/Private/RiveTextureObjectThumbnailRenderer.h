// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/TextureThumbnailRenderer.h"
#include "RiveTextureObjectThumbnailRenderer.generated.h"

class IRiveRenderTarget;
class URiveArtboard;
class URiveTexture;

/**
 *
 */
UCLASS()
class RIVEEDITOR_API URiveTextureObjectThumbnailRenderer
    : public UTextureThumbnailRenderer
{
    GENERATED_BODY()

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
};
