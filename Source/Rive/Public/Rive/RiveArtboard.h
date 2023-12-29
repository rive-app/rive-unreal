// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "UObject/Object.h"
#include "Assets/UnrealRiveFile.h"
#include "RiveArtboard.generated.h"

/**
 *
 */
UCLASS()
class RIVE_API URiveArtboard : public UObject
{
    GENERATED_BODY()

    /**
     * Implementation(s)
     */

public:

    bool LoadNativeArtboard(const TArray<uint8>& InBuffer);

    // Just for testing
    rive::Artboard* GetNativeArtBoard();

    /**
     * Attibute(s)
     */

private:

    UE::Rive::Assets::FUnrealRiveFilePtr UnrealRiveFile;
};
