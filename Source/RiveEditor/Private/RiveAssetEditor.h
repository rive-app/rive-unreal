// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Tools/UAssetEditor.h"
#include "RiveAssetEditor.generated.h"

/**
 * 
 */
UCLASS()
class RIVEEDITOR_API URiveAssetEditor : public UAssetEditor
{
	GENERATED_BODY()

	//~ BEGIN : UAssetEditor Interface

protected:
	
	virtual void GetObjectsToEdit(TArray<UObject*>& OutObjectsToEdit) override;
	
	virtual TSharedPtr<FBaseAssetToolkit> CreateToolkit() override;

	//~ END : UAssetEditor Interface

	/**
	 * Implementation(s)
	 */

public:

	void SetObjectToEdit(UObject* InObject);

	/**
	 * Attribute(s)
	 */

private:
	
	UPROPERTY(Transient)
		TObjectPtr<UObject> ObjectToEdit;
};
