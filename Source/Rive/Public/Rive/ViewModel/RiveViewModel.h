#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_runtime.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveViewModel.generated.h"

namespace rive
{
enum class DataType : unsigned int;
}

UCLASS(BlueprintType)
class RIVE_API URiveViewModel : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(rive::ViewModelRuntime* InViewModel);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModel")
    FString GetName() const;

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModel")
    int32 GetInstanceCount() const;

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModel")
    class URiveViewModelInstance* CreateDefaultInstance() const;

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModel")
    URiveViewModelInstance* CreateInstance();

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModel")
    URiveViewModelInstance* CreateInstanceFromIndex(int32 Index) const;

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModel")
    URiveViewModelInstance* CreateInstanceFromName(const FString& Name) const;

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModel")
    TArray<FString> GetInstanceNames() const;

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModel")
    TArray<FString> GetPropertyNames() const;

    rive::DataType GetPropertyTypeByName(const FString& Name) const;

private:
    URiveViewModelInstance* CreateWrapperInstance(
        rive::ViewModelInstanceRuntime* RuntimeInstance) const;

    rive::ViewModelRuntime* ViewModelRuntimePtr = nullptr;
};