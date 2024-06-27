// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "AssetDefinitionDefault.h"
#include "RiveFileAssetDefinitionDefault.generated.h"

/**
 * 
 */
UCLASS()
class RIVEEDITOR_API URiveFileAssetDefinitionDefault : public UAssetDefinitionDefault
{
	GENERATED_BODY()

	//~ BEGIN : UAssetDefinition Interface

protected:
	
	virtual FText GetAssetDisplayName() const override;

	virtual FLinearColor GetAssetColor() const override;

	virtual TSoftClassPtr<UObject> GetAssetClass() const override;

	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;

	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;

	virtual EAssetCommandResult GetSourceFiles(const FAssetSourceFilesArgs& InArgs, TFunctionRef<bool(const FAssetSourceFilesResult& InSourceFile)> SourceFileFunc) const override;
	//~ END : UAssetDefinition Interface
};
