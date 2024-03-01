// Copyright Rive, Inc. All rights reserved.


#include "RiveTextureThumbnailRenderer.h"

#include "Rive/RiveFile.h"

bool URiveTextureThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	URiveFile* RiveFile = Cast<URiveFile>(Object);
	return RiveFile && RiveFile->IsInitialized() ? UTextureThumbnailRenderer::CanVisualizeAsset(Object) : false;
}

void URiveTextureThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	URiveFile* RiveFile = Cast<URiveFile>(Object);
	if (RiveFile && RiveFile->IsInitialized())
	{
		UTextureThumbnailRenderer::Draw(RiveFile, X, Y, Width, Height, Viewport, Canvas, bAdditionalViewFamily);
	}
}
