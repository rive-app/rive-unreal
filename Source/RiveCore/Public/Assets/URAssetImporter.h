// Copyright Rive, Inc. All rights reserved.

#pragma once
#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/file_asset_loader.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

struct FUREmbeddedAsset;

namespace UE::Rive::Assets
{
	class FURAssetImporter;
	
	/**
	 * Type definition for unique pointer reference to the instance of FURFileAssetLoader.
	 */
	using FUREmbeddedAssetLoaderPtr = TUniquePtr<FURAssetImporter>;
	
	/**
	 * Unreal extension of rive::FileAssetLoader implementation (partial) for the Unreal RHI.
	 */
	class RIVECORE_API FURAssetImporter
#if WITH_RIVE
		final : public rive::FileAssetLoader
#endif // WITH_RIVE
	{
		/**
		 * Structor(s)
		 */

	public:

		FURAssetImporter(const FString& InRiveFilePath, TMap<uint32, FUREmbeddedAsset>& InAssetMap);

#if WITH_RIVE

		//~ BEGIN : rive::FileAssetLoader Interface

	public:
		
		bool loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes, rive::Factory* InFactory) override;

		//~ END : rive::FileAssetLoader Interface

#endif // WITH_RIVE

		/**
		 * Attribute(s)
		 */

	public:

		TMap<uint32, FUREmbeddedAsset>* AssetMap;
		FString RiveFilePath;
	};
}
