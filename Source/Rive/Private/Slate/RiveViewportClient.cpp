// Copyright Rive, Inc. All rights reserved.


#include "RiveViewportClient.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#if WITH_EDITOR
#include "Texture2DPreview.h"
#endif
#include "Rive/RiveFile.h"

UE_DISABLE_OPTIMIZATION

FRiveViewportClient::FRiveViewportClient(URiveFile* InRiveFile, const TSharedRef<SRiveWidgetView>& InWidgetView)
	: RiveFile(InRiveFile)
	, WidgetViewWeakPtr(InWidgetView)
{
}

FRiveViewportClient::~FRiveViewportClient()
{
}

void FRiveViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	if (!RiveFile)
	{
		return;
	}
	
	Canvas->Clear(FLinearColor::Transparent);

	// Now we can draw

	const float MipLevel = 1.f;

	const float LayerIndex = 1.f;
	
	const float SliceIndex = 1.f;
	
	const bool bUsePointSampling = false;

	//todo: how does that work in non-editor builds?
#if WITH_EDITOR
	// TODO. check how that removed from memory
	TRefCountPtr<FBatchedElementParameters> BatchedElementParameters = new FBatchedElementTexture2DPreviewParameters(MipLevel, LayerIndex, SliceIndex, false, false, false, false, false, bUsePointSampling);
#endif
	// Draw the background checkerboard pattern in the same size/position as the render texture so it will show up anywhere
	// the texture has transparency
	// TODO. implement CheckerboardTexture texture in editor mode only, never run it runtime or outside of rive editor
	// FTextureEditorViewportClient::ModifyCheckerboardTextureColors()
	// if (Viewport && CheckerboardTexture)
	// {
	// 	const float CheckerboardSizeX = (float)FMath::Max<int32>(1, CheckerboardTexture->GetSizeX());
	// 	const float CheckerboardSizeY = (float)FMath::Max<int32>(1, CheckerboardTexture->GetSizeY());
	// 	if (Settings.Background == TextureEditorBackground_CheckeredFill)
	// 	{
	// 		Canvas->DrawTile( 0.f, 0.f, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, 0.f, 0.f, (float)Viewport->GetSizeXY().X / CheckerboardSizeX, (float)Viewport->GetSizeXY().Y / CheckerboardSizeY, FLinearColor::White, CheckerboardTexture->GetResource());
	// 	}
	// 	else if (Settings.Background == TextureEditorBackground_Checkered)
	// 	{
	// 		Canvas->DrawTile( XPos, YPos, Width, Height, 0.f, 0.f, (float)Width / CheckerboardSizeX, (float)Height / CheckerboardSizeY, FLinearColor::White, CheckerboardTexture->GetResource());
	// 	}
	// }

	if (RiveFile->GetResource() != nullptr)
	{
		const FIntPoint ViewportSize = Viewport->GetSizeXY();
		
		const FIntPoint RiveTextureSize = RiveFile->CalculateRenderTextureSize(ViewportSize);

		const FIntPoint RiveTexturePosition = RiveFile->CalculateRenderTexturePosition(ViewportSize);
		
		FTexturePlatformData** RunningPlatformDataPtr = RiveFile->GetRunningPlatformData();

		const float Exposure = RunningPlatformDataPtr && *RunningPlatformDataPtr && IsHDR((*RunningPlatformDataPtr)->PixelFormat) ? FMath::Pow(2.f, 0.f) : 1.f;
		
		FCanvasTileItem TileItem(FVector2D(RiveTexturePosition.X, RiveTexturePosition.Y), RiveFile->GetResource(), FVector2D(RiveTextureSize.X, RiveTextureSize.Y), FLinearColor(Exposure, Exposure, Exposure));
		
		TileItem.BlendMode = RiveFile->GetSimpleElementBlendMode(); // TODO. check blending mode
		
#if WITH_EDITOR
		TileItem.BatchedElementParameters = BatchedElementParameters;
#endif

		// Tell canvas to Draw
		Canvas->DrawItem(TileItem);
	}
}

UE_ENABLE_OPTIMIZATION
