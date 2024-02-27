#include "RiveWidgetHelpers.h"

#include "RiveArtboard.h"
#include "Rive/RiveTexture.h"

FVector2f RiveWidgetHelpers::CalculateLocalPointerCoordinatesFromViewport(URiveTexture* InRiveTexture, const URiveArtboard* InArtboard, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2f MouseLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2f ViewportSize = MyGeometry.GetLocalSize();
	const FBox2f TextureBox = CalculateRenderTextureExtentsInViewport(InRiveTexture, ViewportSize);
	return InRiveTexture->GetLocalCoordinatesFromExtents(InArtboard, MouseLocal, TextureBox);
}

FBox2f RiveWidgetHelpers::CalculateRenderTextureExtentsInViewport(URiveTexture* InRiveTexture, const FVector2f& InViewportSize)
{
	const FVector2f RiveTextureSize = {(float)InRiveTexture->SizeX, (float)InRiveTexture->SizeY};
	const float TextureAspectRatio = RiveTextureSize.X / RiveTextureSize.Y;
	const float ViewportAspectRatio = InViewportSize.X / InViewportSize.Y;

	if (ViewportAspectRatio > TextureAspectRatio) // Viewport wider than the Texture => height should be the same
	{
		FVector2f Size {
			InViewportSize.Y * TextureAspectRatio,
			InViewportSize.Y
		};
		float XOffset = (InViewportSize.X - Size.X) * 0.5f;
		return {{XOffset, 0}, {XOffset + Size.X, Size.Y}};
	}
	else // Viewport taller than the Texture => width should be the same
	{
		FVector2f Size {
			(float)InViewportSize.X,
			InViewportSize.X / TextureAspectRatio
		};
		float YOffset = (InViewportSize.Y - Size.Y) * 0.5f;
		return {{0, YOffset}, {Size.X, YOffset + Size.Y}};
	}
}
