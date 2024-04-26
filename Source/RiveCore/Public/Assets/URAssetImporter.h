// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_RIVE
class URiveAsset;
class URiveFile;

#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/file_asset_loader.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE { namespace Rive { namespace Assets
{
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

		FURAssetImporter(UPackage* InPackage, const FString& InRiveFilePath, TMap<uint32, URiveAsset*>& InAssets);

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

		UPackage* RivePackage;
		FString RiveFilePath;
		TMap<uint32, URiveAsset*>& Assets;
	};
}}}
