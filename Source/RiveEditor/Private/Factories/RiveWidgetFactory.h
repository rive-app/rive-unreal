// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UWidgetBlueprint;
class URiveFile;

/**
 * 
 */
class RIVEEDITOR_API FRiveWidgetFactory
{
	/**
	 * Structor(s)
	 */

public:

	FRiveWidgetFactory(URiveFile* InRiveFile);
	
	/**
	 * Implementation(s)
	 */

public:

	bool Create();

private:
	
	UWidgetBlueprint* CreateWidgetBlueprint();
	
	bool SaveAsset(UWidgetBlueprint* InWidgetBlueprint);
	
	bool CreateWidgetStructure(UWidgetBlueprint* InWidgetBlueprint);

private:
	
	URiveFile* RiveFile = nullptr;
};
