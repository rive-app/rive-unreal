#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RiveViewModelPropertyInterface.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_instance_runtime.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveViewModelInstance.generated.h"

class URiveViewModelInstanceValue;

UCLASS(BlueprintType)
class RIVE_API URiveViewModelInstance : public UObject,
                                        public IRiveViewModelPropertyInterface
{
    GENERATED_BODY()

public:
    void Initialize(rive::ViewModelInstanceRuntime* InViewModelInstance);

    void BeginDestroy() override;

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    int32 GetPropertyCount() const;

    template <typename T> T* GetProperty(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    URiveViewModelInstanceBoolean* GetBooleanProperty(
        const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    bool GetBooleanPropertyValue(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    void SetBooleanPropertyValue(const FString& PropertyName, const bool Value);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    URiveViewModelInstanceColor* GetColorProperty(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    FColor GetColorPropertyValue(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    void SetColorPropertyValue(const FString& PropertyName, const FColor Value);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    URiveViewModelInstanceNumber* GetNumberProperty(
        const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    void SetNumberPropertyValue(const FString& PropertyName, const float Value);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    float GetNumberPropertyValue(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    URiveViewModelInstanceString* GetStringProperty(
        const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    void SetStringPropertyValue(const FString& PropertyName,
                                const FString& Value);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    FString GetStringPropertyValue(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    URiveViewModelInstanceEnum* GetEnumProperty(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    FString GetEnumPropertyValue(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    void SetEnumPropertyValue(const FString& PropertyName,
                              const FString& Value);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    TArray<FString> GetEnumPropertyValues(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    URiveViewModelInstanceTrigger* GetTriggerProperty(
        const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    void FireTriggerProperty(const FString& PropertyName);

    UFUNCTION(BlueprintCallable, Category = "Rive|ViewModelInstance")
    URiveViewModelInstance* GetNestedInstanceByName(
        const FString& PropertyName);

    UFUNCTION()
    void AddCallbackProperty(URiveViewModelInstanceValue* Property);
    UFUNCTION()
    void RemoveCallbackProperty(URiveViewModelInstanceValue* Property);
    void HandleCallbacks();
    void ClearCallbacks();

    static FString GetSuffix() { return TEXT("_VIEWMODEL"); }

    rive::ViewModelInstanceRuntime* GetNativePtr() const
    {
        return ViewModelInstancePtr;
    }

private:
    rive::ViewModelInstanceRuntime* ViewModelInstancePtr = nullptr;

    UPROPERTY()
    TMap<FString, UObject*> Properties;

    UPROPERTY()
    TArray<TWeakObjectPtr<URiveViewModelInstanceValue>> CallbackProperties;
};
