// Copyright Rive, Inc. All rights reserved.

#pragma once
#if WITH_RIVE
class URiveFile;
THIRD_PARTY_INCLUDES_START
#include "rive/file_asset_loader.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

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
	class RIVE_API FURAssetImporter
#if WITH_RIVE
		final : public rive::FileAssetLoader
#endif // WITH_RIVE
	{
		/**
		 * Structor(s)
		 */

	public:

		FURAssetImporter(URiveFile* InRiveFile);

#if WITH_RIVE

		//~ BEGIN : rive::FileAssetLoader Interface

	public:
		virtual bool loadContents(rive::FileAsset& InAsset, rive::Span<const uint8> InBandBytes, rive::Factory* InFactory) override;

		//~ END : rive::FileAssetLoader Interface

#endif // WITH_RIVE

		/**
		 * Attribute(s)
		 */

	public:

		URiveFile* RiveFile;
	};
}
