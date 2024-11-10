// Copyright Rive, Inc. All rights reserved.

#include "Rive/Assets/RiveAssetHelpers.h"
#include "Rive/Assets/RiveAsset.h"
#include "Misc/Paths.h"

TArray<FString> RiveAssetHelpers::AssetPaths(
    const FString& InBasePath,
    URiveAsset* InRiveAsset,
    const TArray<FString>& InExtensions)
{
    TArray<FString> Paths;
    FString CombinedPath = FPaths::Combine(InBasePath, InRiveAsset->Name);
    for (auto Extension : InExtensions)
    {
        Paths.Add(FString::Printf(TEXT("%s-%u.%s"),
                                  *CombinedPath,
                                  InRiveAsset->Id,
                                  *Extension));
        Paths.Add(FString::Printf(TEXT("%s.%s"), *CombinedPath, *Extension));
    }

    return Paths;
}

bool RiveAssetHelpers::FindRegistryAsset(const FString& InRiveAssetPath,
                                         const FURAsset& InEmbeddedAsset,
                                         TArray<uint8>& OutAssetBytes)
{
    return false;
}

bool RiveAssetHelpers::FindDiskAsset(const FString& InBasePath,
                                     URiveAsset* InRiveAsset)
{
    // Passed in BasePath will be the Rive file, so we'll parse it out
    FString Directory;
    FString FileName;
    FString Extension;
    FPaths::Split(InBasePath, Directory, FileName, Extension);

    const TArray<FString>* Extensions = nullptr;

    // Just grab our file file extensions first
    switch (InRiveAsset->Type)
    {
        case ERiveAssetType::Font:
            Extensions = &FontExtensions;
            break;
        case ERiveAssetType::Image:
            Extensions = &ImageExtensions;
            break;
        case ERiveAssetType::Audio:
            Extensions = &AudioExtensions;
            break;
        default:
            assert(true);
            return false; // this means we don't support this asset type
    }

    // If we don't have any extensions, we couldn't determine type of asset
    if (Extensions == nullptr)
    {
        return false;
    }

    // Search for the first disk asset match
    bool bHasFileMatch = false;
    FString FilePath;
    TArray<FString> FilePaths = AssetPaths(Directory, InRiveAsset, *Extensions);
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

    InRiveAsset->AssetPath = FilePath;
    return true;
}

ERiveAssetType RiveAssetHelpers::GetUnrealType(uint16_t RiveType)
{
    switch (RiveType)
    {
        case 103:
            return ERiveAssetType::FileBase;
        case 105:
            return ERiveAssetType::Image;
        case 141:
            return ERiveAssetType::Font;
        case 406:
            return ERiveAssetType::Audio;
        default:
            return ERiveAssetType::None;
    }
}
