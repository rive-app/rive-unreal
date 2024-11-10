// Copyright Rive, Inc. All rights reserved.

#include "RiveTextureObjectThumbnailRenderer.h"
#include "Rive/RiveTextureObject.h"

bool URiveTextureObjectThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
    URiveTextureObject* RiveTextureObject = Cast<URiveTextureObject>(Object);
    return RiveTextureObject && RiveTextureObject->IsTickableInEditor();
}

EThumbnailRenderFrequency URiveTextureObjectThumbnailRenderer::
    GetThumbnailRenderFrequency(UObject* Object) const
{
    return EThumbnailRenderFrequency::Realtime;
}

void URiveTextureObjectThumbnailRenderer::Draw(UObject* Object,
                                               int32 X,
                                               int32 Y,
                                               uint32 Width,
                                               uint32 Height,
                                               FRenderTarget* Viewport,
                                               FCanvas* Canvas,
                                               bool bAdditionalViewFamily)
{
    URiveTextureObject* RiveTextureObject = Cast<URiveTextureObject>(Object);
    if (RiveTextureObject && RiveTextureObject->bIsRendering)
    {
        UTextureThumbnailRenderer::Draw(RiveTextureObject,
                                        X,
                                        Y,
                                        Width,
                                        Height,
                                        Viewport,
                                        Canvas,
                                        bAdditionalViewFamily);
    }
}
