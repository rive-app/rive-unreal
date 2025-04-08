#pragma once

#include "CoreMinimal.h"
#include "RiveViewModelInstanceValue.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_instance_enum_runtime.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveViewModelInstanceEnum.generated.h"

namespace rive
{
class ViewModelInstanceEnumRuntime;
}

UCLASS(BlueprintType)
class RIVE_API URiveViewModelInstanceEnum
    : public URiveViewModelInstanceValue,
      public IRiveViewModelPropertyInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|Enum")
    FString GetValue();

    UFUNCTION(BlueprintCallable,
              Category = "Rive|ViewModelInstance|Enum",
              meta = (GetOptions = "GetValuesInternal"))
    void SetValue(const FString& Value);

    UFUNCTION(BlueprintPure)
    void GetValuesInternal(TArray<FString>& Options) const;

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|Enum")
    TArray<FString> GetValues() const;

    static FString GetSuffix() { return TEXT("_ENUM"); }

private:
    rive::ViewModelInstanceEnumRuntime* GetNativePtr() const
    {
        return static_cast<rive::ViewModelInstanceEnumRuntime*>(
            Super::GetNativePtr());
    };
};
