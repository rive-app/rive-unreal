// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/TextureThumbnailRenderer.h"
#include "RiveTextureThumbnailRenderer.generated.h"

class URiveArtboard;
class URiveTexture;

namespace UE::Rive::Renderer
{
	class IRiveRenderTarget;
}
/**
 * 
 */
UCLASS()
class RIVEEDITOR_API URiveTextureThumbnailRenderer : public UTextureThumbnailRenderer
{
	GENERATED_BODY()

	virtual bool CanVisualizeAsset(UObject* Object) override;
	virtual EThumbnailRenderFrequency GetThumbnailRenderFrequency(UObject* Object) const override;
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily) override;

	UPROPERTY()
	URiveTexture* RiveTexture;

	TSharedPtr<UE::Rive::Renderer::IRiveRenderTarget> RiveRenderTarget;

	UPROPERTY()
	URiveArtboard* Artboard;

	bool Initialized = false;
};
