#pragma once

#include "CoreMinimal.h"
#include "RiveViewModelInstanceValue.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_instance_boolean_runtime.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveViewModelInstanceBoolean.generated.h"

UCLASS(BlueprintType)
class RIVE_API URiveViewModelInstanceBoolean
    : public URiveViewModelInstanceValue,
      public IRiveViewModelPropertyInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|Boolean")
    bool GetValue();

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|Boolean")
    void SetValue(bool Value);

    static FString GetSuffix() { return TEXT("_BOOL"); }

private:
    rive::ViewModelInstanceBooleanRuntime* GetNativePtr() const
    {
        return static_cast<rive::ViewModelInstanceBooleanRuntime*>(
            Super::GetNativePtr());
    }
};
