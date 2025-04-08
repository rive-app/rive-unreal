#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RiveViewModelPropertyInterface.h"
#include "RiveViewModelInstanceValue.generated.h"

namespace rive
{
class ViewModelInstanceValueRuntime;
}

class URiveViewModelInstance;

DECLARE_DYNAMIC_DELEGATE(FOnValueChangedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnValueChangedMultiDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAddCallbackPropertyDelegate,
                                  URiveViewModelInstanceValue*,
                                  Value);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnRemoveCallbackPropertyDelegate,
                                  URiveViewModelInstanceValue*,
                                  Value);

UCLASS(BlueprintType)
class RIVE_API URiveViewModelInstanceValue : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(
        rive::ViewModelInstanceValueRuntime* InViewModelInstanceValue);

    void HandleCallbacks();
    void ClearCallbacks();

    UFUNCTION(BlueprintCallable, Category = "Rive")
    void BindToValueChange(FOnValueChangedDelegate Delegate);

    UFUNCTION(BlueprintCallable, Category = "Rive")
    void UnbindFromValueChange(FOnValueChangedDelegate Delegate);

    UFUNCTION(BlueprintCallable, Category = "Rive")
    void UnbindAllFromValueChange();

    UPROPERTY()
    FOnAddCallbackPropertyDelegate OnAddCallbackProperty;
    UPROPERTY()
    FOnRemoveCallbackPropertyDelegate OnRemoveCallbackProperty;

protected:
    virtual void BeginDestroy() override;

    rive::ViewModelInstanceValueRuntime* GetNativePtr() const
    {
        return ViewModelInstanceValuePtr;
    }

    UPROPERTY()
    FOnValueChangedDelegate OnValueChanged;

    FOnValueChangedMultiDelegate OnValueChangedMulti;

private:
    rive::ViewModelInstanceValueRuntime* ViewModelInstanceValuePtr = nullptr;
};
