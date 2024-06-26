// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"

class URiveObject;
class UWidgetBlueprint;
class URiveFile;

/**
 * 
 */
class RIVEEDITOR_API FRiveObjectFactory
{
	/**
	 * Structor(s)
	 */

public:

	FRiveObjectFactory(URiveFile* InRiveFile);
	
	/**
	 * Implementation(s)
	 */

public:

	bool Create();

private:
	
	URiveObject* CreateRiveObject();
	
	bool SaveAsset(URiveObject* InRiveObject);

private:
	
	TObjectPtr<URiveFile> RiveFile;
};
