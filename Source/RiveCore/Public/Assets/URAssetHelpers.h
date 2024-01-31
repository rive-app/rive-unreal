// Copyright Rive, Inc. All rights reserved.

#pragma once

struct FUREmbeddedAsset;

class URAssetHelpers
{
public:
	static TArray<FString> AssetPaths(const FString& InBasePath, const FString& InAssetName, uint32_t InAssetId, const TArray<FString>& InExtensions);

    static bool FindRegistryAsset(const FString& InRiveAssetPath, const FUREmbeddedAsset& InEmbeddedAsset, TArray<uint8>& OutAssetBytes);

    static bool FindDiskAsset(const FString& InBasePath, const FUREmbeddedAsset& InEmbeddedAsset, FString& OutPath); //TArray<uint8>& OutAssetBytes)

    static bool LoadDiskAsset(const FString& InAssetPath, FUREmbeddedAsset& InEmbeddedAsset);

	inline const static TArray<FString> FontExtensions = {"ttf", "otf"};
	inline const static TArray<FString> ImageExtensions = {"png"};
};
