#pragma once

class URiveAsset;
struct FURAsset;

class URAssetHelpers
{
public:
	static TArray<FString> AssetPaths(const FString& InBasePath, URiveAsset* InRiveAsset, const TArray<FString>& InExtensions);

    static bool FindRegistryAsset(const FString& InRiveAssetPath, const FURAsset& InEmbeddedAsset, TArray<uint8>& OutAssetBytes);

    static bool FindDiskAsset(const FString& InBasePath, URiveAsset* InRiveAsset); //TArray<uint8>& OutAssetBytes)

	inline const static TArray<FString> FontExtensions = {"ttf", "otf"};
	inline const static TArray<FString> ImageExtensions = {"png"};
};
