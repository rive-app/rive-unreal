// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Assets/UnrealRiveFileAssetLoader.h"
#include "Core/UnrealRiveFactory.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/file.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Assets
{
    class FUnrealRiveFile;

    /**
     * Type definition for unique pointer reference to the instance of FUnrealRiveFile.
     */
    using FUnrealRiveFilePtr = TUniquePtr<FUnrealRiveFile>;

    /**
     * Represents a Rive File containing Artboards, StateMachines, and Animations.
     *
     * There are higher level behaviours that you can use directly in the Unreal editor. Use this class if you need direct control of the lifecycle of the Rive File.
     */
    class RIVECORE_API FUnrealRiveFile
    {
        /**
         * Structor(s)
         */

    public:
        
        FUnrealRiveFile(const TArray<uint8>& InBuffer);

        FUnrealRiveFile(const uint8* InBytes, uint32 InSize);

        ~FUnrealRiveFile() = default;

#if WITH_RIVE

        /**
         * Implementation(s)
         */

    public:

        rive::ImportResult Import(Core::FUnrealRiveFactory* InRiveFactory, Assets::FUnrealRiveFileAssetLoader* InAssetLoader = nullptr);

    	// Just for testing
    	rive::Artboard* GetNativeArtBoard();

    private:

        void PrintStats();

        /**
         * Attribute(s)
         */

    private:

        FUnrealRiveFileAssetLoaderPtr FileAssetLoader;

        std::unique_ptr<rive::File> NativeFilePtr;

        rive::Span<const uint8> NativeFileSpan;

    	std::unique_ptr<rive::ArtboardInstance> ArtboardInstance;

#endif // WITH_RIVE
    };
}
