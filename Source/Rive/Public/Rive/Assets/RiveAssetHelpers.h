// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

enum class ERiveAssetType : uint8;
class URiveAsset;
struct FURAsset;

class RiveAssetHelpers
{
public:
    static TArray<FString> AssetPaths(const FString& InBasePath,
                                      URiveAsset* InRiveAsset,
                                      const TArray<FString>& InExtensions);

    static bool FindRegistryAsset(const FString& InRiveAssetPath,
                                  const FURAsset& InEmbeddedAsset,
                                  TArray<uint8>& OutAssetBytes);

    static bool FindDiskAsset(
        const FString& InBasePath,
        URiveAsset* InRiveAsset); // TArray<uint8>& OutAssetBytes)

    static ERiveAssetType GetUnrealType(uint16_t RiveType);

    inline const static TArray<FString> FontExtensions = {"ttf", "otf"};
    inline const static TArray<FString> ImageExtensions = {"png"};
    inline const static TArray<FString> AudioExtensions = {"wav",
                                                           "mp3",
                                                           "flac"};
};
