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
	return RiveFile && RiveFile->IsInitialized();
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


		if (!ThumbnailRenderers.Contains(RiveFile->GetFName()))
		{
			FRiveThumbnailData Data;
			Data.RiveTexture = NewObject<URiveTexture>(),
			Data.RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(GetFName(), Data.RiveTexture);
			Data.RiveRenderTarget->SetClearColor(FLinearColor::Transparent);
			Data.RiveTexture->ResizeRenderTargets(FIntPoint(Width, Height));
			Data.RiveRenderTarget->Initialize();
			Data.Artboard = NewObject<URiveArtboard>();
			Data.Artboard->Initialize(RiveFile->GetNativeFile(), Data.RiveRenderTarget);
			RiveFile->Artboards.Add(Data.Artboard);

			ThumbnailRenderers.Add(RiveFile->GetFName(), Data);
		}
	

		FRiveThumbnailData* ThumbnailData = ThumbnailRenderers.Find(Object->GetFName());
		if (ThumbnailData != nullptr)
		{
			ThumbnailData->RiveRenderTarget->Save();
			ThumbnailData->Artboard->Align(ERiveFitType::ScaleDown, ERiveAlignment::Center);
			ThumbnailData->Artboard->Tick(FApp::GetDeltaTime());
			ThumbnailData->RiveRenderTarget->Restore();
			ThumbnailData->RiveRenderTarget->SubmitAndClear();
			UTextureThumbnailRenderer::Draw(ThumbnailData->RiveTexture, X, Y, Width, Height, Viewport, Canvas, bAdditionalViewFamily);
		}
	}
}
