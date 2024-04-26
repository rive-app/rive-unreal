// Copyright Rive, Inc. All rights reserved.

#include "RiveWidgetHelpers.h"

#include "RiveArtboard.h"
#include "Rive/RiveTexture.h"
#include "Layout/Geometry.h"
#include "Input/Events.h"

FVector2D RiveWidgetHelpers::CalculateLocalPointerCoordinatesFromViewport(URiveTexture* InRiveTexture, URiveArtboard* InArtboard, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D MouseLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D ViewportSize = MyGeometry.GetLocalSize();
	const FBox2D TextureBox = CalculateRenderTextureExtentsInViewport(InRiveTexture->Size, ViewportSize);
	return InRiveTexture->GetLocalCoordinatesFromExtents(InArtboard, MouseLocal, TextureBox);
}

FBox2D RiveWidgetHelpers::CalculateRenderTextureExtentsInViewport(const FVector2D& InTextureSize, const FVector2D& InViewportSize)
{
	const float TextureAspectRatio = InTextureSize.X / InTextureSize.Y;
	const float ViewportAspectRatio = InViewportSize.X / InViewportSize.Y;

	if (ViewportAspectRatio > TextureAspectRatio) // Viewport wider than the Texture => height should be the same
	{
		FVector2D Size {
			InViewportSize.Y * TextureAspectRatio,
			InViewportSize.Y
		};
		float XOffset = (InViewportSize.X - Size.X) * 0.5f;
		return {{XOffset, 0}, {XOffset + Size.X, Size.Y}};
	}
	else // Viewport taller than the Texture => width should be the same
	{
		FVector2D Size {
			(float)InViewportSize.X,
			InViewportSize.X / TextureAspectRatio
		};
		float YOffset = (InViewportSize.Y - Size.Y) * 0.5f;
		return {{0, YOffset}, {Size.X, YOffset + Size.Y}};
	}
}
