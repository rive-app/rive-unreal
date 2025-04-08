#pragma once

#include "CoreMinimal.h"
#include "RiveViewModelInstanceValue.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_instance_color_runtime.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveViewModelInstanceColor.generated.h"

namespace rive
{
class ViewModelInstanceColorRuntime;
}

UCLASS(BlueprintType)
class RIVE_API URiveViewModelInstanceColor
    : public URiveViewModelInstanceValue,
      public IRiveViewModelPropertyInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|Color")
    FColor GetColor();

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance|Color")
    void SetColor(FColor Color);

    static FString GetSuffix() { return TEXT("_COLOR"); }

private:
    rive::ViewModelInstanceColorRuntime* GetNativePtr() const
    {
        return static_cast<rive::ViewModelInstanceColorRuntime*>(
            Super::GetNativePtr());
    };
};
