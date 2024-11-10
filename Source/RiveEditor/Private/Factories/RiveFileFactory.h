// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "EditorReimportHandler.h"
#include "Factories/Factory.h"
#include "RiveFileFactory.generated.h"

/**
 *
 */
UCLASS(HideCategories = Object, MinimalAPI)
class URiveFileFactory : public UFactory, public FReimportHandler
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
    virtual UObject* FactoryCreateFile(UClass* InClass,
                                       UObject* InParent,
                                       FName InName,
                                       EObjectFlags Flags,
                                       const FString& Filename,
                                       const TCHAR* Params,
                                       FFeedbackContext* Warn,
                                       bool& bOutOperationCanceled) override;
    //~ END : UFactory Interface

    //~ BEGIN : FReimportHandler Interface
public:
    virtual bool CanReimport(UObject* Obj,
                             TArray<FString>& OutFilenames) override;
    virtual void SetReimportPaths(
        UObject* Obj,
        const TArray<FString>& NewReimportPaths) override;
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
    virtual int32 GetPriority() const override;
    //~ END : FReimportHandler Interface
};
