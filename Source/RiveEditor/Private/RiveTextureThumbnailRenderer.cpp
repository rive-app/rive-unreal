// Copyright Rive, Inc. All rights reserved.


#include "RiveTextureThumbnailRenderer.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "RiveArtboard.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveTexture.h"

bool URiveTextureThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	URiveFile* RiveFile = Cast<URiveFile>(Object);
	return RiveFile && RiveFile->IsInitialized() && (!Initialized || Artboard->bIsInitialized);
}

EThumbnailRenderFrequency URiveTextureThumbnailRenderer::GetThumbnailRenderFrequency(UObject* Object) const
{
	return EThumbnailRenderFrequency::Realtime;
}

void URiveTextureThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	if (URiveFile* RiveFile = Cast<URiveFile>(Object))
	{

		UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
		if (!RiveRenderer)
		{
			return;
		}

		if (!Initialized)
		{
			Initialized = true;
			RiveTexture = NewObject<URiveTexture>();
			RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(GetFName(), RiveTexture);
			RiveRenderTarget->SetClearColor(FLinearColor::Transparent);
			RiveTexture->ResizeRenderTargets(FIntPoint(Width, Height));
			RiveRenderTarget->Initialize();

			Artboard = NewObject<URiveArtboard>();
			Artboard->Initialize(RiveFile->GetNativeFile(), RiveRenderTarget);
			RiveFile->Artboards.Add(Artboard);
		} else
		{
			RiveRenderTarget->Save();
			Artboard->Align(ERiveFitType::ScaleDown, ERiveAlignment::Center);
			Artboard->Tick(FApp::GetDeltaTime());
			RiveRenderTarget->Restore();
			RiveRenderTarget->SubmitAndClear();
		}

		if (RiveTexture)
		{
			UTextureThumbnailRenderer::Draw(RiveTexture, X, Y, Width, Height, Viewport, Canvas, bAdditionalViewFamily);
		}
	}
}
