#pragma once

#include "CoreMinimal.h"
#include "RiveViewModelInstanceValue.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_instance_string_runtime.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveViewModelInstanceString.generated.h"

UCLASS(BlueprintType)
class RIVE_API URiveViewModelInstanceString
    : public URiveViewModelInstanceValue,
      public IRiveViewModelPropertyInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|String")
    FString GetValue();

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|String")
    void SetValue(const FString& Value);

    static FString GetSuffix() { return TEXT("_STRING"); }

private:
    rive::ViewModelInstanceStringRuntime* GetNativePtr() const
    {
        return static_cast<rive::ViewModelInstanceStringRuntime*>(
            Super::GetNativePtr());
    }
};
