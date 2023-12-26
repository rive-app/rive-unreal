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
public:
	FRiveWidgetFactory(URiveFile* InRiveFile);

	bool Create();

private:
	UWidgetBlueprint* CreateWidgetBlueprint();
	bool SaveAsset(UWidgetBlueprint* InWidgetBlueprint);
	bool CreateWidgetStructure(UWidgetBlueprint* InWidgetBlueprint);

private:
	TObjectPtr<URiveFile> RiveFile;
};
