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
	const FIntPoint RiveTextureSize = RiveFile->CalculateRenderTextureSize(ViewportSize);
	const FIntPoint RiveTexturePosition = RiveFile->CalculateRenderTexturePosition(ViewportSize, RiveTextureSize);

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
			Canvas->DrawTile(RiveTexturePosition.X, RiveTexturePosition.Y,
				RiveTextureSize.X, RiveTextureSize.Y,
				0.f, 0.f,
				(float)RiveTextureSize.X / CheckerboardSizeX, (float)RiveTextureSize.Y / CheckerboardSizeY,
				FLinearColor::White,
				CheckerboardTexture->GetResource());
		}
		else if (Settings.Background == TextureEditorBackground_Checkered)
		{
			Canvas->DrawTile( RiveTexturePosition.X, RiveTexturePosition.Y,
			RiveTextureSize.X, RiveTextureSize.Y,
			0.f, 0.f,
			(float)RiveTextureSize.X / CheckerboardSizeX, (float)RiveTextureSize.Y / CheckerboardSizeY,
			FLinearColor::White,
			CheckerboardTexture->GetResource());
		}
	}
#endif
	
	if (RiveFile->GetResource() != nullptr)
	{
		FCanvasTileItem TileItem(FVector2D{RiveTexturePosition},
			RiveFile->GetResource(),
			FVector2D{RiveTextureSize},
			FLinearColor::White);
		TileItem.BlendMode = SE_BLEND_AlphaBlend;
		TileItem.BatchedElementParameters = nullptr;
		
		Canvas->DrawItem(TileItem);
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
