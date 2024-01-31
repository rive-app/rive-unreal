// Copyright Rive, Inc. All rights reserved.

#include "Assets/URAssetHelpers.h"

#include "Assets/URAssetImporter.h"
#include "Assets/UREmbeddedAsset.h"
#include "Logs/RiveCoreLog.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/generated/assets/font_asset_base.hpp"
#include "rive/generated/assets/image_asset_base.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

TArray<FString> URAssetHelpers::AssetPaths(const FString& InBasePath, const FString& InAssetName, uint32_t InAssetId,
                                           const TArray<FString>& InExtensions)
{
	TArray<FString> Paths;
	FString CombinedPath = FPaths::Combine(InBasePath, InAssetName);
	for (auto Extension : InExtensions)
	{
		Paths.Add(FString::Printf(TEXT("%s-%u.%s"), *CombinedPath, InAssetId, *Extension));
		Paths.Add(FString::Printf(TEXT("%s.%s"), *CombinedPath, *Extension));

	}

	return Paths;
}

bool URAssetHelpers::FindRegistryAsset(const FString& InRiveAssetPath,
	const FUREmbeddedAsset& InEmbeddedAsset, TArray<uint8>& OutAssetBytes)
{
	return false;
}

bool URAssetHelpers::FindDiskAsset(const FString& InBasePath, const FUREmbeddedAsset& InEmbeddedAsset,
	FString& OutPath)
{
#if WITH_RIVE

	// Passed in BasePath will be the Rive file, so we'll parse it out
	FString Directory;
	FString FileName;
	FString Extension;
	FPaths::Split(InBasePath, Directory, FileName, Extension);

	const TArray<FString>* Extensions = nullptr;

	// Just grab our file file extensions first
	switch (InEmbeddedAsset.Type)
	{
	case rive::FontAssetBase::typeKey:
		Extensions = &FontExtensions;
		break;
	case rive::ImageAssetBase::typeKey:
		Extensions = &ImageExtensions;
		break;
	default:
		return false;  // this means we don't
	}

	// If we don't have any extensions, we couldn't determine type of asset
	if (Extensions == nullptr)
	{
		return false;
	}

	// Search for the first disk asset match
	bool bHasFileMatch = false;
	FString FilePath;
	TArray<FString> FilePaths = AssetPaths(Directory, FileName, InEmbeddedAsset.Id, *Extensions);
	for (const FString& Path : FilePaths)
	{
		if (FPaths::FileExists(Path))
		{
			FilePath = Path;
			bHasFileMatch = true;
			break;
		}
	}

	if (!bHasFileMatch)
	{
		// We couldn't find any disk assets that match
		return false;
	}

	OutPath = FilePath;
   
#endif // WITH_RIVE

	return true;
}

bool URAssetHelpers::LoadDiskAsset(const FString& InAssetPath, FUREmbeddedAsset& InEmbeddedAsset)
{
	if (!FFileHelper::LoadFileToArray(InEmbeddedAsset.Bytes, *InAssetPath))
	{
		UE_LOG(LogRiveCore, Error, TEXT("Could not load Asset: %s at path %s"), *InEmbeddedAsset.Name, *InAssetPath);
		return false;
	}

	return true;
}
