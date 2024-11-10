// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"

class URiveTextureObject;
class UWidgetBlueprint;
class URiveFile;

/**
 *
 */
class RIVEEDITOR_API FRiveTextureObjectFactory
{
    /**
     * Structor(s)
     */

public:
    FRiveTextureObjectFactory(URiveFile* InRiveFile);

    /**
     * Implementation(s)
     */

public:
    bool Create();

private:
    URiveTextureObject* CreateRiveTextureObject();

    bool SaveAsset(URiveTextureObject* InRiveObject);

private:
    TObjectPtr<URiveFile> RiveFile;
};
