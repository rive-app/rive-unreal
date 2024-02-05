// Copyright Rive, Inc. All rights reserved.

#include "Assets/URAssetImporter.h"

#include "Assets/URAssetHelpers.h"
#include "Assets/UREmbeddedAsset.h"
#include "Assets/URFileAssetLoader.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/assets/file_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#if WITH_RIVE

UE::Rive::Assets::FURAssetImporter::FURAssetImporter(const FString& InRiveFilePath, TMap<uint32, FUREmbeddedAsset>& InAssetMap)
	: AssetMap(&InAssetMap), RiveFilePath(InRiveFilePath)
{
}

bool UE::Rive::Assets::FURAssetImporter::loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes, rive::Factory* InFactory)
{
	bool bIsInBand = InBandBytes.size() > 0;
	
	FUREmbeddedAsset EmbeddedAsset = {
		InAsset.coreType(),
		InAsset.assetId(),
		UTF8_TO_TCHAR(InAsset.name().c_str()),
		bIsInBand,
		InBandBytes.size()
	};
	
	if (bIsInBand)
	{
		AssetMap->Add(InAsset.assetId(), EmbeddedAsset);
		return true;
	}
	
	FString AssetPath;
	if (URAssetHelpers::FindDiskAsset(RiveFilePath, EmbeddedAsset, AssetPath))
	{
		EmbeddedAsset.AssetPath = AssetPath;
		URAssetHelpers::LoadDiskAsset(AssetPath, EmbeddedAsset);
	}

	AssetMap->Add(InAsset.assetId(), EmbeddedAsset);
	return true;
}

#endif // WITH_RIVE
