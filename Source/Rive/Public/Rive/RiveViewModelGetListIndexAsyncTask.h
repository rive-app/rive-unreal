// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "RiveViewModelGetListIndexAsyncTask.generated.h"

class URiveViewModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FRiveViewModelGetListIndexAsyncTaskOutput,
    int32,
    ResultValue,
    URiveViewModel*,
    ViewModel);
/**
 *  Gets the list index for a given view model if available.
 *  Fails if no index is available or if the view model is invalid.
 */
UCLASS()
class RIVE_API URiveViewModelGetListIndexAsyncTask
    : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable)
    FRiveViewModelGetListIndexAsyncTaskOutput OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FRiveViewModelGetListIndexAsyncTaskOutput OnFailure;

    UFUNCTION(BlueprintCallable,
              meta = (BlueprintInternalUseOnly = "true"),
              Category = "AsyncTasks",
              DisplayName = "GetViewModelListIndex")
    static URiveViewModelGetListIndexAsyncTask* RunGetListIndexAsyncTaskAsync(
        URiveViewModel* ViewModel);

    virtual void Activate() override;

private:
    // The view model to get the index of.
    UPROPERTY(Transient)
    URiveViewModel* ViewModel;
};
