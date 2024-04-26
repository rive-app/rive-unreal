// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class URiveArtboard;
class URiveTexture;
struct FPointerEvent;
struct FGeometry;

class RiveWidgetHelpers
{
public:
	static FVector2D CalculateLocalPointerCoordinatesFromViewport(URiveTexture* InRiveTexture, URiveArtboard* InArtboard, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	/**
	 * Calculates the extents of the RiveFIle RenderTarget to be 'Contained' within the given viewport
	 */
	static FBox2D CalculateRenderTextureExtentsInViewport(const FVector2D& TextureSize, const FVector2D& InViewportSize);
};
