// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "URAssetImporter.h"
#include "Assets/URFileAssetLoader.h"
#include "Core/URArtboard.h"

#if WITH_RIVE
class URiveFile;
THIRD_PARTY_INCLUDES_START
#include "rive/file.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Assets
{
	class FURAsset;
	class FURFile;

    /**
     * Type definition for unique pointer reference to the instance of FURFile.
     */
    using FURFilePtr = TUniquePtr<FURFile>;

    /**
     * Represents a Rive File containing Artboards, StateMachines, and Animations.
     *
     * There are higher level behaviours that you can use directly in the Unreal editor. Use this class if you need direct control of the lifecycle of the Rive File.
     */
    class RIVECORE_API FURFile
    {
        /**
         * Structor(s)
         */

    public:
        
        FURFile(const TArray<uint8>& InBuffer);

        FURFile(const uint8* InBytes, uint32 InSize);

        ~FURFile() = default;

#if WITH_RIVE

        /**
         * Implementation(s)
         */

    public:

		rive::ImportResult EditorImport(const FString& InRiveFilePath, TMap<uint32, FUREmbeddedAsset>& InAssetMap, rive::Factory* InRiveFactory);
        rive::ImportResult Import(TMap<uint32, FUREmbeddedAsset>& InAssetMap, rive::Factory* InRiveFactory);

        Core::FURArtboard* GetArtboard() const;

    private:

        void PrintStats();

        /**
         * Attribute(s)
         */

    private:

        Core::FURArtboardPtr Artboard;

        FURFileAssetLoaderPtr FileAssetLoader;

        std::unique_ptr<rive::File> NativeFilePtr;

        rive::Span<const uint8> NativeFileSpan;

#endif // WITH_RIVE
    };
}
