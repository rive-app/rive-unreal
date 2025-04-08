#pragma once

#include "CoreMinimal.h"
#include "RiveViewModelInstanceValue.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_instance_number_runtime.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveViewModelInstanceNumber.generated.h"

namespace rive
{
class ViewModelInstanceNumberRuntime;
}

UCLASS(BlueprintType)
class RIVE_API URiveViewModelInstanceNumber
    : public URiveViewModelInstanceValue,
      public IRiveViewModelPropertyInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|Number")
    float GetValue();

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|Number")
    void SetValue(float Value);

    static FString GetSuffix() { return TEXT("_NUMBER"); }

private:
    rive::ViewModelInstanceNumberRuntime* GetNativePtr() const
    {
        return static_cast<rive::ViewModelInstanceNumberRuntime*>(
            Super::GetNativePtr());
    };
};
