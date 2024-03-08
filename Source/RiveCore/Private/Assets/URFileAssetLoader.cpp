// Copyright Rive, Inc. All rights reserved.

#include "Assets/URFileAssetLoader.h"
#include "Assets/RiveAsset.h"
#include "Logs/RiveCoreLog.h"
#include "rive/factory.hpp"

#if WITH_RIVE
#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/assets/file_asset.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE::Rive::Assets::FURFileAssetLoader::FURFileAssetLoader(UObject* InOuter, TMap<uint32, TObjectPtr<URiveAsset>>& InAssets)
	: Outer(InOuter), Assets(InAssets)
{
}

#if WITH_RIVE

bool UE::Rive::Assets::FURFileAssetLoader::loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes,
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
	// Unity version prefers disk assets, over InBand, if they exist, allowing overrides
	
	URiveAsset* RiveAsset = nullptr;
	const TObjectPtr<URiveAsset>* RiveAssetPtr = Assets.Find(InAsset.assetId());
	if (RiveAssetPtr == nullptr)
	{
		if (!bUseInBand)
		{
			UE_LOG(LogRiveCore, Error, TEXT("Could not find pre-loaded asset. This means the initial import probably failed."));
			return false;
		}
		
		RiveAsset = NewObject<URiveAsset>(Outer, URiveAsset::StaticClass(),
			MakeUniqueObjectName(Outer, URiveAsset::StaticClass(), FName{FString::Printf(TEXT("%d"),InAsset.assetId())}),
			RF_Transient);

		RiveAsset->Id = InAsset.assetId();
		RiveAsset->Name = FString(UTF8_TO_TCHAR(InAsset.name().c_str()));
		RiveAsset->Type = static_cast<ERiveAssetType>(InAsset.coreType());
		RiveAsset->bIsInBand = true;
	}
	else
	{
		RiveAsset = RiveAssetPtr->Get();
	}

	rive::Span<const uint8> OutOfBandBytes;
	if (!bUseInBand)
	{
		if (RiveAsset->NativeAssetBytes.IsEmpty())
		{
			UE_LOG(LogRiveCore, Error, TEXT("Trying to load out of band asset, but its bytes were never filled."));
			return false;
		}
		OutOfBandBytes = rive::make_span(RiveAsset->NativeAssetBytes.GetData(), RiveAsset->NativeAssetBytes.Num());
		AssetBytes = &OutOfBandBytes;
	}

	return RiveAsset->DecodeNativeAsset(InAsset, InFactory, *AssetBytes);
}

#endif // WITH_RIVE
