// Copyright Rive, Inc. All rights reserved.

#include "Assets/URFileAssetLoader.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/assets/file_asset.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE::Rive::Assets::FURFileAssetLoader::FURFileAssetLoader(const FString& InRiveFilePath)
    : RelativePath(InRiveFilePath)
{
    if (!InRiveFilePath.IsEmpty())
    {
        RelativePath = FPaths::GetPath(InRiveFilePath);
    }
}

#if WITH_RIVE

bool UE::Rive::Assets::FURFileAssetLoader::loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes, rive::Factory* InFactory)
{
    bool bDidLoad = false;

    FAssetRef* AssetIter = nullptr;

    if (Assets.Contains(InAsset.assetId()))
    {
        AssetIter = Assets.Find(InAsset.assetId());
    }
    else
    {
        FAssetRef NewAsset = {InAsset.coreType(), FString(InAsset.uniqueFilename().c_str()), InAsset.as<rive::Asset>()};

        AssetIter = &Assets.Add(InAsset.assetId(), NewAsset);
    }

    if (rive::Asset* Asset = static_cast<rive::Asset*>(AssetIter->Asset))
    {
        if (Asset->coreType() == AssetIter->Type)
        {
            switch (Asset->coreType())
            {
                case rive::FontAssetBase::typeKey:
                {
                    rive::FontAsset* FontAsset = Asset->as<rive::FontAsset>();

                    FontAsset->font(rive::ref_rcp(static_cast<rive::Font*>(AssetIter->Asset)));

                    bDidLoad = true;

                    break;
                }
                case rive::ImageAssetBase::typeKey:
                {
                    rive::ImageAsset* ImageAsset = Asset->as<rive::ImageAsset>();

                    ImageAsset->renderImage(rive::ref_rcp(static_cast<rive::RenderImage*>(AssetIter->Asset)));

                    bDidLoad = true;

                    break;
                }
            }
        }
    }

    return bDidLoad;
}

#endif // WITH_RIVE
