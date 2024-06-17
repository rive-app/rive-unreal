﻿// Copyright Rive, Inc. All rights reserved.


#include "RiveTextureThumbnailRenderer.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveTexture.h"

URiveTextureThumbnailRenderer::URiveTextureThumbnailRenderer() : RiveTexture(nullptr)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().OnAssetRemoved().AddUObject(this, &URiveTextureThumbnailRenderer::OnAssetRemoved);
}

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
		IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

		if (!Initialized)
		{
			Initialized = true;
			RiveTexture = NewObject<URiveTexture>(),
			RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(GetFName(), RiveTexture);
			RiveRenderTarget->SetClearColor(FLinearColor::Transparent);
			RiveTexture->ResizeRenderTargets(FIntPoint(Width, Height));
			RiveRenderTarget->Initialize();
		}

		URiveArtboard* Artboard = nullptr;
		if (!ThumbnailRenderers.Contains(RiveFile->GetFName()))
		{
			Artboard = NewObject<URiveArtboard>();
			Artboard->Initialize(RiveFile->GetNativeFile(), RiveRenderTarget);
			RiveFile->Artboards.Add(Artboard);
			ThumbnailRenderers.Add(RiveFile->GetFName(), Artboard);
		} else
		{
			URiveArtboard** Found = ThumbnailRenderers.Find(Object->GetFName());
			if (Found != nullptr)
			{
				Artboard = *Found;
			}
		}
		
		if (Artboard != nullptr)
		{
			RiveRenderTarget->Save();
			Artboard->Align(ERiveFitType::ScaleDown, ERiveAlignment::Center);
			Artboard->Tick(FApp::GetDeltaTime());
			RiveRenderTarget->Restore();
			RiveRenderTarget->SubmitAndClear();
			UTextureThumbnailRenderer::Draw(RiveTexture, X, Y, Width, Height, Viewport, Canvas, bAdditionalViewFamily);
		}
	}
}

void URiveTextureThumbnailRenderer::OnAssetRemoved(const FAssetData& AssetData)
{
	for (TTuple<FName, URiveArtboard*>& ThumbnailRenderer : ThumbnailRenderers)
	{
		if (ThumbnailRenderer.Key == AssetData.AssetName)
		{
			ThumbnailRenderers.Remove(AssetData.AssetName);
			return;
		}
	}
}
