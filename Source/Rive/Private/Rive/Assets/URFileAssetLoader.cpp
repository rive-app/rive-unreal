// Copyright Rive, Inc. All rights reserved.

#include "Rive/Assets/URFileAssetLoader.h"

#include "Logs/RiveLog.h"
#include "rive/factory.hpp"
#include "Rive/RiveFile.h"
#include "Rive/Assets/RiveAsset.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/assets/file_asset.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE::Rive::Assets::FURFileAssetLoader::FURFileAssetLoader(URiveFile* InRiveFile) : RiveFile(InRiveFile)
{
}

#if WITH_RIVE

bool UE::Rive::Assets::FURFileAssetLoader::loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes,
                                                        rive::Factory* InFactory)
{
	rive::Span<const uint8>* AssetBytes = &InBandBytes;
	bool bUseInBand = InBandBytes.size() > 0;


	// We can take two paths here
	// 1. Either search for a file to load off disk (or in the Unreal registry)
	// 2. Or use InBandbytes if no other options are found to load
	// Unity version prefers disk assets, over InBand, if they exist, allowing overrides

	URiveAsset* RiveAsset = nullptr;
	TObjectPtr<URiveAsset>* RiveAssetPtr = RiveFile->Assets.Find(InAsset.assetId());
	if (RiveAssetPtr == nullptr)
	{
		if (!bUseInBand)
		{
			UE_LOG(LogRive, Error, TEXT("Could not find pre-loaded asset. This means the initial import probably failed."));
			return false;
		}

		RiveAsset = NewObject<URiveAsset>(RiveFile, URiveAsset::StaticClass(), FName(FString::Printf(TEXT("%d"), InAsset.assetId())), RF_Transient);
	} else
	{
		RiveAsset = RiveAssetPtr->Get();
	}

	rive::Span<const uint8> OutOfBandBytes;
	if (!bUseInBand)
	{
		if (RiveAsset->NativeAssetBytes.IsEmpty())
		{
			UE_LOG(LogRive, Error, TEXT("Trying to load out of band asset, but its bytes were never filled."));
			return false;
		}
		OutOfBandBytes = rive::make_span(RiveAsset->NativeAssetBytes.GetData(), RiveAsset->NativeAssetBytes.Num());
		AssetBytes = &OutOfBandBytes;
	}

	return RiveAsset->DecodeNativeAsset(InAsset, InFactory, *AssetBytes);
}

#endif // WITH_RIVE
