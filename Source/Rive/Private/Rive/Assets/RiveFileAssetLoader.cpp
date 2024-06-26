// Copyright Rive, Inc. All rights reserved.

#include "Rive/Assets/RiveFileAssetLoader.h"

#include "Logs/RiveLog.h"
#include "Rive/Assets/RiveAsset.h"
#include "Rive/Assets/RiveAssetHelpers.h"
#include "rive/factory.hpp"

#if WITH_RIVE
#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/assets/file_asset.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

FRiveFileAssetLoader::FRiveFileAssetLoader(UObject* InOuter, TMap<uint32, TObjectPtr<URiveAsset>>& InAssets)
	: Outer(InOuter), Assets(InAssets)
{
}

#if WITH_RIVE

bool FRiveFileAssetLoader::loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes,
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
	if (RiveAssetPtr != nullptr && RiveAssetPtr->Get() != nullptr)
	{
		RiveAsset = RiveAssetPtr->Get();
	}
	else
	{
		if (!bUseInBand)
		{
			UE_LOG(LogRive, Error, TEXT("Could not find pre-loaded asset. This means the initial import probably failed."));
			return false;
		}
		
		RiveAsset = NewObject<URiveAsset>(Outer, URiveAsset::StaticClass(),
			MakeUniqueObjectName(Outer, URiveAsset::StaticClass(), FName{FString::Printf(TEXT("%d"),InAsset.assetId())}),
			RF_Transient);

		RiveAsset->Id = InAsset.assetId();
		RiveAsset->Name = FString(UTF8_TO_TCHAR(InAsset.name().c_str()));
		RiveAsset->Type = RiveAssetHelpers::GetUnrealType(InAsset.coreType()); 
		RiveAsset->bIsInBand = true;

		// We only add it to our assets here so that it shows up in the inspector, otherwise this doesn't have any functional effect on anything due to it being a transient, in-band asset
		Assets.Add(InAsset.assetId(), RiveAsset);
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

	return RiveAsset->LoadNativeAsset(InAsset, InFactory, *AssetBytes);
}

#endif // WITH_RIVE
