// Copyright Rive, Inc. All rights reserved.

#include "Rive/Assets/RiveFileAssetImporter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logs/RiveLog.h"
#include "Misc/FileHelper.h"
#include "Rive/Assets/RiveAsset.h"
#include "Rive/Assets/RiveAssetHelpers.h"
#include "Rive/Assets/RiveFileAssetLoader.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#endif

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/assets/file_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#if WITH_RIVE

FRiveFileAssetImporter::FRiveFileAssetImporter(
    UPackage* InPackage,
    const FString& InRiveFilePath,
    TMap<uint32, TObjectPtr<URiveAsset>>& InAssets) :
    RivePackage(InPackage), RiveFilePath(InRiveFilePath), Assets(InAssets)
{}

bool FRiveFileAssetImporter::loadContents(rive::FileAsset& InAsset,
                                          rive::Span<const uint8> InBandBytes,
                                          rive::Factory* InFactory)
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

    FString AssetName =
        FString::Printf(TEXT("%s-%u"),
                        *FString(UTF8_TO_TCHAR(InAsset.name().c_str())),
                        InAsset.assetId());

    // TODO: We should just go ahead and search Registry for this asset, and
    // load it here
    {
        // There shouldn't be anything in RiveFile->Assets, as this
        // AssetImporter is meant to only be called on the Riv first load.
        TObjectPtr<URiveAsset>* RiveAssetPtr = Assets.Find(InAsset.assetId());

        if (RiveAssetPtr)
        {
            assert(false);
            return false;
        }
    }

    ERiveAssetType Type = RiveAssetHelpers::GetUnrealType(InAsset.coreType());

    URiveAsset* RiveAsset = nullptr;

    if (!bIsInBand)
    {
        const FString RivePackageName = RivePackage->GetName();
        const FString RiveFolderName =
            *FPackageName::GetLongPackagePath(RivePackageName);

        const FString RiveAssetPath = FString::Printf(TEXT("%s/%s.%s"),
                                                      *RiveFolderName,
                                                      *AssetName,
                                                      *AssetName);

        // Determine if this asset already exists at this path
        FAssetRegistryModule& AssetRegistryModule =
            FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
                "AssetRegistry");
        FAssetData AssetData =
            AssetRegistryModule.Get().GetAssetByObjectPath(RiveAssetPath);
        if (AssetData.IsValid())
        {
            RiveAsset = Cast<URiveAsset>(AssetData.GetAsset());
        }
        else
        {
#if WITH_EDITOR
            FAssetToolsModule& AssetToolsModule =
                FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(
                    "AssetTools");
            RiveAsset = Cast<URiveAsset>(
                AssetToolsModule.Get().CreateAsset(AssetName,
                                                   RiveFolderName,
                                                   URiveAsset::StaticClass(),
                                                   nullptr));
#endif
        }
    }
    else
    {
        RiveAsset = NewObject<URiveAsset>(nullptr, URiveAsset::StaticClass());
    }

    if (!RiveAsset)
    {
        UE_LOG(LogRive, Error, TEXT("Could not load the RiveAsset."));
        return false;
    }

    RiveAsset->Id = InAsset.assetId();
    RiveAsset->Name = FString(UTF8_TO_TCHAR(InAsset.name().c_str()));
    RiveAsset->Type = Type;
    RiveAsset->bIsInBand = bIsInBand;
    RiveAsset->MarkPackageDirty();

    if (bIsInBand)
    {
        Assets.Add(InAsset.assetId(), RiveAsset);
        return true;
    }

    // Search for the oob asset on disk, relative to the Rive File path
    FString AssetPath;
    if (RiveAssetHelpers::FindDiskAsset(RiveFilePath, RiveAsset))
    {
        if (!FFileHelper::LoadFileToArray(RiveAsset->NativeAssetBytes,
                                          *AssetPath))
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("Could not load Asset: %s at path %s"),
                   *RiveAsset->Name,
                   *AssetPath);
        }
    }

    Assets.Add(InAsset.assetId(), RiveAsset);
    return true;
}

#endif // WITH_RIVE
