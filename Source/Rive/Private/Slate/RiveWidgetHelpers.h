#pragma once

class URiveArtboard;
class URiveTexture;

class RiveWidgetHelpers
{
public:
	static FVector2f CalculateLocalPointerCoordinatesFromViewport(URiveTexture* InRiveTexture, const URiveArtboard* InArtboard, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	/**
	 * Calculates the extents of the RiveFIle RenderTarget to be 'Contained' within the given viewport
	 */
	static FBox2f CalculateRenderTextureExtentsInViewport(URiveTexture* InRiveTexture, const FVector2f& InViewportSize);
};
