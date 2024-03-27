// Copyright Rive, Inc. All rights reserved.

#include "RiveViewportClient.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "RiveWidgetHelpers.h"
#include "Rive/RiveTexture.h"
#include "RiveArtboard.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"


#if WITH_EDITOR // For Checkerboard Texture
#include "TextureEditorSettings.h"
#include "ImageUtils.h"
#endif

FRiveViewportClient::FRiveViewportClient(URiveTexture* InRiveTexture, const TArray<URiveArtboard*> InArtboards, const TSharedRef<SRiveWidgetView>& InWidgetView)
	: RiveTexture(InRiveTexture)
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
	if (!IsValid(RiveTexture))
	{
		return;
	}
	
	Canvas->Clear(FLinearColor::Transparent);

	//todo: to review with drawing of multiple artboards
	const FIntPoint ViewportSize = Viewport->GetSizeXY();
	const FBox2f RiveTextureBox = RiveWidgetHelpers::CalculateRenderTextureExtentsInViewport(RiveTexture->Size, ViewportSize);
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
			Canvas->DrawTile(0.0f, 0.0f, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, 0.0f, 0.0f, (float)Viewport->GetSizeXY().X / CheckerboardSizeX, (float)Viewport->GetSizeXY().Y / CheckerboardSizeY, FLinearColor::White, CheckerboardTexture->GetResource());
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
	
	if (RiveTexture->GetResource() != nullptr)
	{
		FCanvasTileItem TileItem(FVector2D{RiveTextureBox.Min},
			RiveTexture->GetResource(),
			FVector2D{RiveTextureSize},
			FLinearColor::White);
		TileItem.BlendMode = RiveTexture->GetSimpleElementBlendMode();
		TileItem.BatchedElementParameters = nullptr;
		
		Canvas->DrawItem(TileItem);
	}
}

void FRiveViewportClient::SetRiveTexture(URiveTexture* InRiveTexture)
{
	RiveTexture = InRiveTexture;
}

void FRiveViewportClient::RegisterArtboardInputs(const TArray<URiveArtboard*>& InArtboards)
{
}

#if WITH_EDITOR
void FRiveViewportClient::ModifyCheckerboardTextureColors()
{
	DestroyCheckerboardTexture();

	const UTextureEditorSettings& Settings = *GetDefault<UTextureEditorSettings>();
	CheckerboardTexture = FImageUtils::CreateCheckerboardTexture(Settings.CheckerColorOne, Settings.CheckerColorTwo, Settings.CheckerSize);
	CheckerboardTexture->AddToRoot();
}

void FRiveViewportClient::DestroyCheckerboardTexture()
{
	if (IsValid(CheckerboardTexture))
	{
		CheckerboardTexture->ReleaseResource();
		CheckerboardTexture->RemoveFromRoot();
		CheckerboardTexture->MarkAsGarbage();
		CheckerboardTexture = nullptr;
	}
}
#endif
