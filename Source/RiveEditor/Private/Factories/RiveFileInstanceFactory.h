// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "RiveFileInstanceFactory.generated.h"

/**
 * 
 */
UCLASS()
class RIVEEDITOR_API URiveFileInstanceFactory : public UFactory
{
	GENERATED_BODY()

public:

	explicit URiveFileInstanceFactory(const FObjectInitializer& ObjectInitializer);
	
	/** An initial rive file to place in the newly created material */
	UPROPERTY()
	TObjectPtr<class URiveFile> InitialRiveFile;

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* InClass,UObject* InParent,FName InName,EObjectFlags InFlags,UObject* Context,FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface
};
