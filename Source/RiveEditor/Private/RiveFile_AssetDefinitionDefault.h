// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "AssetDefinitionDefault.h"
#include "RiveFile_AssetDefinitionDefault.generated.h"

/**
 * 
 */
UCLASS()
class RIVEEDITOR_API URiveFile_AssetDefinitionDefault : public UAssetDefinitionDefault
{
	GENERATED_BODY()

	//~ BEGIN : UAssetDefinition Interface

protected:

	virtual FText GetAssetDisplayName() const override;

	virtual FLinearColor GetAssetColor() const override;

	virtual TSoftClassPtr<UObject> GetAssetClass() const override;

	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;

	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;

	//~ END : UAssetDefinition Interface
};
