// Copyright Rive, Inc. All rights reserved.

#include "Assets/URFileAssetLoader.h"
#include "Assets/UREmbeddedAsset.h"
#include "Logs/RiveCoreLog.h"
#include "rive/factory.hpp"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/assets/file_asset.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE::Rive::Assets::FURFileAssetLoader::FURFileAssetLoader(TMap<uint32, FUREmbeddedAsset>& InAssetMap)
	: AssetMap(&InAssetMap)
{
}

#if WITH_RIVE

bool UE::Rive::Assets::FURFileAssetLoader::loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes,
                                                        rive::Factory* InFactory)
{
	bool bDidLoad = false;

	rive::Span<const uint8>* AssetBytes = &InBandBytes;
	bool bUseInBand = InBandBytes.size() > 0;


	// We can take two paths here
	// 1. Either search for a file to load off disk (or in the Unreal registry)
	// 2. Or use InBandbytes if no other options are found to load
	// Unity version prefers disk assets, over InBand, if they exist, allowing overrides

	FUREmbeddedAsset* Asset = AssetMap->Find(InAsset.assetId());
	if (Asset == nullptr)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Could not find pre-loaded asset. This means the initial import probably failed."));
		return false;
	}

	rive::Span<const uint8> OutOfBandBytes;
	if (!bUseInBand)
	{
		OutOfBandBytes = rive::make_span(Asset->Bytes.GetData(), Asset->Bytes.Num());
		AssetBytes = &OutOfBandBytes;
	}
	
	switch (Asset->Type)
	{
	case rive::FontAssetBase::typeKey:
		{
			rive::rcp<rive::Font> DecodedFont = InFactory->decodeFont(*AssetBytes);

			if (DecodedFont == nullptr)
			{
				UE_LOG(LogRiveCore, Error, TEXT("Could not decode font asset: %s"), *Asset->Name);
				break;
			}

			rive::FontAsset* FontAsset = InAsset.as<rive::FontAsset>();
			FontAsset->font(DecodedFont);
			// EmbeddedAsset.Asset = FontAsset;
			Asset->Asset = FontAsset;
			bDidLoad = true;
			break;
		}
	case rive::ImageAssetBase::typeKey:
		{
			rive::rcp<rive::RenderImage> DecodedImage = InFactory->decodeImage(*AssetBytes);

			if (DecodedImage == nullptr)
			{
				UE_LOG(LogRiveCore, Error, TEXT("Could not decode image asset: %s"), *Asset->Name);
				break;
			}

			rive::ImageAsset* ImageAsset = InAsset.as<rive::ImageAsset>();
			ImageAsset->renderImage(DecodedImage);
			Asset->Asset = ImageAsset;
			bDidLoad = true;
			break;
		}
	}

	if (bDidLoad)
	{
		UE_LOG(LogRiveCore, Log, TEXT("Loaded Asset: %s"), *Asset->Name);
	}
	return bDidLoad;
}

#endif // WITH_RIVE
