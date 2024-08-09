// Fill out your copyright notice in the Description page of Project Settings.

#include "Rive/Assets/RiveImageAsset.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"

#include "PreRiveHeaders.h"
#include "rive/pls/pls_render_context_impl.hpp"
THIRD_PARTY_INCLUDES_START
#include "rive/factory.hpp"
#include "rive/pls/pls_render_context.hpp"
THIRD_PARTY_INCLUDES_END


URiveImageAsset::URiveImageAsset()
{
	Type = ERiveAssetType::Image;
}

void URiveImageAsset::LoadTexture(UTexture2D* InTexture)
{
	if (!InTexture) return;

	IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

	RiveRenderer->CallOrRegister_OnInitialized(IRiveRenderer::FOnRendererInitialized::FDelegate::CreateLambda(
		[this, InTexture](IRiveRenderer* RiveRenderer)
		{
			rive::pls::PLSRenderContext* PLSRenderContext;
			{
				FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
				PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr();
			}

			if (ensure(PLSRenderContext))
			{
				InTexture->SetForceMipLevelsToBeResident(30.f);
				InTexture->WaitForStreaming();

				EPixelFormat PixelFormat = InTexture->GetPixelFormat();
				if (PixelFormat != PF_R8G8B8A8)
				{
					UE_LOG(LogRive, Error, TEXT("Error loading Texture '%s': Rive only supports RGBA pixel formats. This texture is of format"), *InTexture->GetName())
					return;
				}

				FTexture2DMipMap& Mip = InTexture->GetPlatformData()->Mips[0];
				uint8* MipData = reinterpret_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_ONLY));
				int32 BitmapDataSize = Mip.SizeX * Mip.SizeY * sizeof(FColor);

				TArray<uint8> BitmapData;
				BitmapData.AddUninitialized(BitmapDataSize);
				FMemory::Memcpy(BitmapData.GetData(), MipData, BitmapDataSize);
				Mip.BulkData.Unlock();

				if (MipData == nullptr)
				{
					UE_LOG(LogRive, Error, TEXT("Unable to load Mip data for %s"), *InTexture->GetName());
					return;
				}


				// decodeImage() here requires encoded bytes and returns a rive::RenderImage
				// it will call:
				//   PLSRenderContextHelperImpl::decodeImageTexture() ->
				//     Bitmap::decode()
				//       // here, Bitmap only decodes webp, jpg, png and discards otherwise
				rive::rcp<rive::RenderImage> DecodedImage = PLSRenderContext->decodeImage(rive::make_span(BitmapData.GetData(), BitmapDataSize));

				// This is what we need, to make a RenderImage and supply raw bitmap bytes that aren't already encoded:
				// makeImage, createImage, or any other descriptive name could be used
				// rive::rcp<rive::RenderImage> RenderImage = PLSRenderContext->makeImage(rive::make_span(BitmapData.GetData(), BitmapDataSize));

				if (DecodedImage == nullptr)
				{
					UE_LOG(LogRive, Error, TEXT("Could not decode image asset: %s"), *InTexture->GetName());
					return;
				}

				NativeAsset->as<rive::ImageAsset>()->renderImage(DecodedImage);
			}
		}
	));
}

bool URiveImageAsset::LoadNativeAssetBytes(rive::FileAsset& InAsset, rive::Factory* InRiveFactory, const rive::Span<const uint8>& AssetBytes)
{
	rive::rcp<rive::RenderImage> DecodedImage = InRiveFactory->decodeImage(AssetBytes);

	if (DecodedImage == nullptr)
	{
		UE_LOG(LogRive, Error, TEXT("Could not decode image asset: %s"), *Name);
		return false;
	}

	rive::ImageAsset* ImageAsset = InAsset.as<rive::ImageAsset>();
	ImageAsset->renderImage(DecodedImage);
	NativeAsset = ImageAsset;
	return true;
}
