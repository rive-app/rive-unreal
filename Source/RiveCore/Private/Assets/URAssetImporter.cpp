// Copyright Rive, Inc. All rights reserved.

#include "Assets/URAssetImporter.h"
#include "Logs/RiveCoreLog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Assets/RiveAsset.h"
#include "Assets/URAssetHelpers.h"
#include "Assets/URFileAssetLoader.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#endif

#if WITH_RIVE
#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/assets/file_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#if WITH_RIVE

UE::Rive::Assets::FURAssetImporter::FURAssetImporter(UPackage* InPackage, const FString& InRiveFilePath, TMap<uint32, TObjectPtr<URiveAsset>>& InAssets)
	: RivePackage(InPackage), RiveFilePath(InRiveFilePath), Assets(InAssets)
{
}

bool UE::Rive::Assets::FURAssetImporter::loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes, rive::Factory* InFactory)
{
	// Just proceed to load the base type without processing it
	if (InAsset.coreType() == rive::FileAssetBase::typeKey)
	{
		return true;
	}
	
	bool bIsInBand = InBandBytes.size() > 0;
	if (bIsInBand)
	{
		// Ignore in band for imports, it'll always be loaded on the fly
		return true;
	}

	FString AssetName = FString::Printf(TEXT("%s-%u"),
		*FString(UTF8_TO_TCHAR(InAsset.name().c_str())),
		InAsset.assetId());

	// TODO: We should just go ahead and search Registry for this asset, and load it here
	{
		// There shouldn't be anything in RiveFile->Assets, as this AssetImporter is meant to only be called on the Riv first load.
		TObjectPtr<URiveAsset>* RiveAssetPtr = Assets.Find(InAsset.assetId());

		if (RiveAssetPtr)
		{
			assert(false);
			return false;
		}
	}


	URiveAsset* RiveAsset = nullptr;
	
	if (!bIsInBand)
	{
		const FString RivePackageName = RivePackage->GetName();
		const FString RiveFolderName = *FPackageName::GetLongPackagePath(RivePackageName);

		const FString RiveAssetPath = FString::Printf(TEXT("%s/%s.%s"), *RiveFolderName, *AssetName, *AssetName);
		
		// Determine if this asset already exists at this path
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(RiveAssetPath);
		if (AssetData.IsValid())
		{
			RiveAsset = Cast<URiveAsset>(AssetData.GetAsset());
		}
		else
		{
#if WITH_EDITOR
			FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
			RiveAsset = Cast<URiveAsset>(AssetToolsModule.Get().CreateAsset(AssetName, RiveFolderName, URiveAsset::StaticClass(), nullptr));
#endif
		}
	} else
	{
		RiveAsset = NewObject<URiveAsset>(nullptr, URiveAsset::StaticClass());
	}

	if (!RiveAsset)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Could not load the RiveAsset."));
		return false;
	}
	
	RiveAsset->Id = InAsset.assetId();
	RiveAsset->Name = FString(UTF8_TO_TCHAR(InAsset.name().c_str()));
	RiveAsset->Type = static_cast<ERiveAssetType>(InAsset.coreType());
	RiveAsset->bIsInBand = bIsInBand;
	RiveAsset->MarkPackageDirty();
	
	if (bIsInBand)
	{
		Assets.Add(InAsset.assetId(), RiveAsset);
		return true;
	}
	
	FString AssetPath;
	if (URAssetHelpers::FindDiskAsset(RiveFilePath, RiveAsset))
	{
		RiveAsset->LoadFromDisk();
	}

	Assets.Add(InAsset.assetId(), RiveAsset);
	return true;
}

#endif // WITH_RIVE
