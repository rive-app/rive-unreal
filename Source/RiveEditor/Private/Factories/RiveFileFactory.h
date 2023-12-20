// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Factories/Factory.h"
#include "RiveFileFactory.generated.h"

/**
 *
 */
UCLASS(HideCategories = Object, MinimalAPI)
class URiveFileFactory : public UFactory
{
    GENERATED_BODY()

    /**
     * Structor(s)
     */

public:

    explicit URiveFileFactory(const FObjectInitializer& ObjectInitializer);

    //~ BEGIN : UFactory Interface

public:

    virtual bool FactoryCanImport(const FString& Filename) override;

    virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

    //~ END : UFactory Interface
};
