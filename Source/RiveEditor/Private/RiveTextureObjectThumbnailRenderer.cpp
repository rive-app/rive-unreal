// Copyright Rive, Inc. All rights reserved.


#include "RiveTextureObjectThumbnailRenderer.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveTexture.h"
#include "Rive/RiveTextureObject.h"


bool URiveTextureObjectThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	URiveTextureObject* RiveTextureObject = Cast<URiveTextureObject>(Object);
	return RiveTextureObject && RiveTextureObject->IsTickableInEditor();
}

EThumbnailRenderFrequency URiveTextureObjectThumbnailRenderer::GetThumbnailRenderFrequency(UObject* Object) const
{
	return EThumbnailRenderFrequency::Realtime;
}

void URiveTextureObjectThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Viewport, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	URiveTextureObject* RiveTextureObject = Cast<URiveTextureObject>(Object);
	if (RiveTextureObject && RiveTextureObject->bIsRendering)
	{
		UTextureThumbnailRenderer::Draw(RiveTextureObject, X, Y, Width, Height, Viewport, Canvas, bAdditionalViewFamily);
	}	// if (URiveFile* RiveFile = Cast<URiveFile>(Object))
	// {
	// 	IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
	//
	// 	if (!Initialized)
	// 	{
	// 		Initialized = true;
	// 		RiveTexture = NewObject<URiveTexture>(),
	// 		RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(GetFName(), RiveTexture);
	// 		RiveRenderTarget->SetClearColor(FLinearColor::Transparent);
	// 		RiveTexture->ResizeRenderTargets(FIntPoint(Width, Height));
	// 		RiveRenderTarget->Initialize();
	// 	}
	//
	// 	URiveArtboard* Artboard = nullptr;
	// 	if (!ThumbnailRenderers.Contains(RiveFile->GetFName()))
	// 	{
	// 		Artboard = NewObject<URiveArtboard>();
	// 		Artboard->Initialize(RiveFile->GetNativeFile(), RiveRenderTarget);
	// 		RiveFile->Artboards.Add(Artboard);
	// 		ThumbnailRenderers.Add(RiveFile->GetFName(), Artboard);
	// 	} else
	// 	{
	// 		URiveArtboard** Found = ThumbnailRenderers.Find(Object->GetFName());
	// 		if (Found != nullptr)
	// 		{
	// 			Artboard = *Found;
	// 		}
	// 	}
	// 	
	// 	if (Artboard != nullptr)
	// 	{
	// 		RiveRenderTarget->Save();
	// 		Artboard->Align(ERiveFitType::ScaleDown, ERiveAlignment::Center);
	// 		Artboard->Tick(FApp::GetDeltaTime());
	// 		RiveRenderTarget->Restore();
	// 		RiveRenderTarget->SubmitAndClear();
	// 		UTextureThumbnailRenderer::Draw(RiveTexture, X, Y, Width, Height, Viewport, Canvas, bAdditionalViewFamily);
	// 	}
	// }
}
