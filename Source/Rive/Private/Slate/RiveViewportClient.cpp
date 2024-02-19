// Copyright Rive, Inc. All rights reserved.

#include "RiveViewportClient.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Rive/RiveFile.h"

#if WITH_EDITOR // For Checkerboard Texture
#include "TextureEditorSettings.h"
#include "ImageUtils.h"
#endif

FRiveViewportClient::FRiveViewportClient(URiveFile* InRiveFile, const TSharedRef<SRiveWidgetView>& InWidgetView)
	: RiveFile(InRiveFile)
	, WidgetViewWeakPtr(InWidgetView)
{
#if WITH_EDITOR
	ModifyCheckerboardTextureColors();
#endif
}

FRiveViewportClient::~FRiveViewportClient()
{
#if WITH_EDITOR
	DestroyCheckerboardTexture();
#endif
}

void FRiveViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	if (!RiveFile)
	{
		return;
	}
	
	Canvas->Clear(FLinearColor::Transparent);

	//todo: to review with drawing of multiple artboards
	const FIntPoint ViewportSize = Viewport->GetSizeXY();
	const FBox2f RiveTextureBox = CalculateRenderTextureExtentsInViewport(ViewportSize);
	const FVector2f RiveTextureSize = RiveTextureBox.GetSize();

#if WITH_EDITOR
	// Draw the background checkerboard pattern in the same size/position as the render texture so it will show up anywhere
	// the texture has transparency
	if (Viewport && bDrawCheckeredTexture && IsValid(CheckerboardTexture))
	{
		const UTextureEditorSettings& Settings = *GetDefault<UTextureEditorSettings>();
 		const float CheckerboardSizeX = (float)FMath::Max<int32>(1, CheckerboardTexture->GetSizeX());
		const float CheckerboardSizeY = (float)FMath::Max<int32>(1, CheckerboardTexture->GetSizeY());
		if (Settings.Background == TextureEditorBackground_CheckeredFill)
		{
			Canvas->DrawTile(RiveTextureBox.Min.X, RiveTextureBox.Min.Y,
				RiveTextureSize.X, RiveTextureSize.Y,
				0.f, 0.f,
				RiveTextureSize.X / CheckerboardSizeX, RiveTextureSize.Y / CheckerboardSizeY,
				FLinearColor::White,
				CheckerboardTexture->GetResource());
		}
		else if (Settings.Background == TextureEditorBackground_Checkered)
		{
			Canvas->DrawTile( RiveTextureBox.Min.X, RiveTextureBox.Min.Y,
			RiveTextureSize.X, RiveTextureSize.Y,
			0.f, 0.f,
			RiveTextureSize.X / CheckerboardSizeX, RiveTextureSize.Y / CheckerboardSizeY,
			FLinearColor::White,
			CheckerboardTexture->GetResource());
		}
	}
#endif
	
	if (RiveFile->GetResource() != nullptr)
	{
		FCanvasTileItem TileItem(FVector2D{RiveTextureBox.Min},
			RiveFile->GetResource(),
			FVector2D{RiveTextureSize},
			FLinearColor::White);
		TileItem.BlendMode = SE_BLEND_AlphaBlend;
		TileItem.BatchedElementParameters = nullptr;
		
		Canvas->DrawItem(TileItem);
	}
}

FVector2f FRiveViewportClient::CalculateLocalPointerCoordinatesFromViewport(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const FVector2f MouseLocal = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2f ViewportSize = MyGeometry.GetLocalSize();
	const FBox2f TextureBox = CalculateRenderTextureExtentsInViewport(ViewportSize);
	const FVector2f LocalPosition = RiveFile->GetLocalCoordinatesFromExtents(MouseLocal, TextureBox);
	return LocalPosition;
}

FBox2f FRiveViewportClient::CalculateRenderTextureExtentsInViewport(const FVector2f& InViewportSize) const
{
	if (!IsValid(RiveFile))
	{
		return {};
	}
	
	const FVector2f ArtboardSize = {(float)RiveFile->SizeX, (float)RiveFile->SizeY};
	const float TextureAspectRatio = ArtboardSize.X / ArtboardSize.Y;
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

#if WITH_EDITOR
void FRiveViewportClient::ModifyCheckerboardTextureColors()
{
	DestroyCheckerboardTexture();

	const UTextureEditorSettings& Settings = *GetDefault<UTextureEditorSettings>();
	CheckerboardTexture = FImageUtils::CreateCheckerboardTexture(Settings.CheckerColorOne, Settings.CheckerColorTwo, Settings.CheckerSize);
}

void FRiveViewportClient::DestroyCheckerboardTexture()
{
	if (CheckerboardTexture)
	{
		if (CheckerboardTexture->GetResource())
		{
			CheckerboardTexture->ReleaseResource();
		}
		CheckerboardTexture->MarkAsGarbage();
		CheckerboardTexture = nullptr;
	}
}
#endif
