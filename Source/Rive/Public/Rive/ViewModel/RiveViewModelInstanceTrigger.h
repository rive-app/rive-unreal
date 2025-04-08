#pragma once

#include "CoreMinimal.h"
#include "RiveViewModelInstanceValue.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_instance_trigger_runtime.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveViewModelInstanceTrigger.generated.h"

UCLASS(BlueprintType)
class RIVE_API URiveViewModelInstanceTrigger
    : public URiveViewModelInstanceValue,
      public IRiveViewModelPropertyInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|Trigger")
    void Trigger();

    static FString GetSuffix() { return TEXT("_TRIGGER"); }

private:
    rive::ViewModelInstanceTriggerRuntime* GetNativePtr() const
    {
        return static_cast<rive::ViewModelInstanceTriggerRuntime*>(
            Super::GetNativePtr());
    };
};
