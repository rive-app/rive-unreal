// Copyright Rive, Inc. All rights reserved.

#include "Rive/Assets/RiveFileAssetLoader.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
#include "Logs/RiveLog.h"
#include "Rive/Assets/RiveAsset.h"
#include "Rive/Assets/RiveAssetHelpers.h"
#include "Rive/Assets/RiveAudioAsset.h"
#include "Rive/Assets/RiveFontAsset.h"
#include "Rive/Assets/RiveImageAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/factory.hpp"
#include "rive/assets/file_asset.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Private::RiveFileAssetLoader
{
bool NeedsClassReplacement(URiveAsset* RiveAsset)
{
    switch (RiveAsset->Type)
    {
        case ERiveAssetType::Font:
        {
            if (!Cast<URiveFontAsset>(RiveAsset))
            {
                return true;
            }
            break;
        }

        case ERiveAssetType::Audio:
        {
            if (!Cast<URiveAudioAsset>(RiveAsset))
            {
                return true;
            }
            break;
        }
        case ERiveAssetType::Image:
        {
            if (!Cast<URiveImageAsset>(RiveAsset))
            {
                return true;
            }
            break;
        }
        default:
            break;
    }

    return false;
}

URiveAsset* ReplaceAsset(UObject* Outer, URiveAsset* RiveAsset)
{
    URiveAsset* NewAsset = nullptr;
    switch (RiveAsset->Type)
    {
        case ERiveAssetType::Font:
            NewAsset = NewObject<URiveFontAsset>(Outer,
                                                 URiveFontAsset::StaticClass(),
                                                 NAME_None,
                                                 RF_Public | RF_Standalone);
            break;
        case ERiveAssetType::Audio:
            NewAsset =
                NewObject<URiveAudioAsset>(Outer,
                                           URiveAudioAsset::StaticClass(),
                                           NAME_None,
                                           RF_Public | RF_Standalone);
            break;
        case ERiveAssetType::Image:
            NewAsset =
                NewObject<URiveImageAsset>(Outer,
                                           URiveImageAsset::StaticClass(),
                                           NAME_None,
                                           RF_Public | RF_Standalone);
            break;
        default:
            break;
    }

    if (!NewAsset)
        return nullptr;

    UEngine::CopyPropertiesForUnrelatedObjects(RiveAsset, NewAsset);

    UPackage* Package = RiveAsset->GetOutermost();
    FString AssetPath = Package->GetPathName();
    UPackage* NewPackage = CreatePackage(*AssetPath);

    FString Name = RiveAsset->GetName();
    const ERenameFlags PkgRenameFlags =
        REN_ForceNoResetLoaders | REN_DoNotDirty | REN_DontCreateRedirectors |
        REN_NonTransactional | REN_SkipGeneratedClasses;
    RiveAsset->Rename(*FString::Printf(TEXT("%s_Old"), *Name),
                      Package,
                      PkgRenameFlags);
    RiveAsset->ClearFlags(RF_Standalone | RF_Public);
    RiveAsset->MarkAsGarbage();

    NewAsset->Rename(*Name, NewPackage);

#if WITH_EDITOR
    FAssetRegistryModule::AssetDeleted(RiveAsset);
    FAssetRegistryModule::AssetCreated(NewAsset);
    NewAsset->MarkPackageDirty();

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(NewPackage,
                          NewAsset,
                          *FPackageName::LongPackageNameToFilename(
                              AssetPath,
                              FPackageName::GetAssetPackageExtension()),
                          SaveArgs);
#endif

    return NewAsset;
}

} // namespace UE::Private::RiveFileAssetLoader

FRiveFileAssetLoader::FRiveFileAssetLoader(
    UObject* InOuter,
    TMap<uint32, TObjectPtr<URiveAsset>>& InAssets) :
    Outer(InOuter), Assets(InAssets)
{}

#if WITH_RIVE

bool FRiveFileAssetLoader::loadContents(rive::FileAsset& InAsset,
                                        rive::Span<const uint8> InBandBytes,
                                        rive::Factory* InFactory)
{
    // Just proceed to load the base type without processing it
    if (InAsset.coreType() == rive::FileAssetBase::typeKey)
    {
        return true;
    }

    const rive::Span<const uint8>* AssetBytes = &InBandBytes;
    const bool bUseInBand = InBandBytes.size() > 0;

    // We can take two paths here
    // 1. Either search for a file to load off disk (or in the Unreal registry)
    // 2. Or use InBandbytes if no other options are found to load
    // Unity version prefers disk assets, over InBand, if they exist, allowing
    // overrides

    URiveAsset* RiveAsset = nullptr;
    const TObjectPtr<URiveAsset>* RiveAssetPtr = Assets.Find(InAsset.assetId());
    if (RiveAssetPtr != nullptr && RiveAssetPtr->Get() != nullptr)
    {
        RiveAsset = RiveAssetPtr->Get();

        if (UE::Private::RiveFileAssetLoader::NeedsClassReplacement(RiveAsset))
        {
            if (URiveAsset* NewAsset =
                    UE::Private::RiveFileAssetLoader::ReplaceAsset(Outer,
                                                                   RiveAsset))
            {
                RiveAsset = NewAsset;
                Assets[InAsset.assetId()] = NewAsset;
            }
        }
    }
    else
    {
        if (!bUseInBand)
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("Could not find pre-loaded asset. This means the "
                        "initial import probably "
                        "failed."));
            return false;
        }

        ERiveAssetType Type =
            RiveAssetHelpers::GetUnrealType(InAsset.coreType());
        switch (Type)
        {
            case ERiveAssetType::Audio:
                RiveAsset = NewObject<URiveAudioAsset>(
                    Outer,
                    URiveAudioAsset::StaticClass(),
                    MakeUniqueObjectName(
                        Outer,
                        URiveAudioAsset::StaticClass(),
                        FName{FString::Printf(TEXT("%d"), InAsset.assetId())}),
                    RF_Transient);
                break;
            case ERiveAssetType::Font:
                RiveAsset = NewObject<URiveFontAsset>(
                    Outer,
                    URiveFontAsset::StaticClass(),
                    MakeUniqueObjectName(
                        Outer,
                        URiveFontAsset::StaticClass(),
                        FName{FString::Printf(TEXT("%d"), InAsset.assetId())}),
                    RF_Transient);
                break;
            case ERiveAssetType::Image:
                RiveAsset = NewObject<URiveImageAsset>(
                    Outer,
                    URiveImageAsset::StaticClass(),
                    MakeUniqueObjectName(
                        Outer,
                        URiveImageAsset::StaticClass(),
                        FName{FString::Printf(TEXT("%d"), InAsset.assetId())}),
                    RF_Transient);
                break;
            default:
                RiveAsset = NewObject<URiveAsset>(
                    Outer,
                    URiveAsset::StaticClass(),
                    MakeUniqueObjectName(
                        Outer,
                        URiveAsset::StaticClass(),
                        FName{FString::Printf(TEXT("%d"), InAsset.assetId())}),
                    RF_Transient);
                break;
        }

        RiveAsset->Id = InAsset.assetId();
        RiveAsset->Name = FString(UTF8_TO_TCHAR(InAsset.name().c_str()));
        RiveAsset->Type = Type;
        RiveAsset->bIsInBand = true;

        // We only add it to our assets here so that it shows up in the
        // inspector, otherwise this doesn't have any functional effect on
        // anything due to it being a transient, in-band asset
        Assets.Add(InAsset.assetId(), RiveAsset);
    }

    rive::Span<const uint8> OutOfBandBytes;
    if (!bUseInBand)
    {
        if (RiveAsset->NativeAssetBytes.IsEmpty())
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("Trying to load out of band asset, but its bytes were "
                        "never filled."));
            return false;
        }
        OutOfBandBytes = rive::make_span(RiveAsset->NativeAssetBytes.GetData(),
                                         RiveAsset->NativeAssetBytes.Num());
        AssetBytes = &OutOfBandBytes;
    }

    return RiveAsset->LoadNativeAssetBytes(InAsset, InFactory, *AssetBytes);
}

#endif // WITH_RIVE
