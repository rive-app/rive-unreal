#include "Rive/ViewModel/RiveViewModelInstance.h"
#include "Rive/ViewModel/RiveViewModelInstanceNumber.h"
#include "Rive/ViewModel/RiveViewModelInstanceString.h"
#include "Rive/ViewModel/RiveViewModelInstanceTrigger.h"
#include "Rive/ViewModel/RiveViewModelInstanceBoolean.h"
#include "Rive/ViewModel/RiveViewModelInstanceColor.h"
#include "Rive/ViewModel/RiveViewModelInstanceEnum.h"
#include "Rive/ViewModel/RiveViewModelPropertyResolver.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveUtils.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_instance_runtime.hpp"
THIRD_PARTY_INCLUDES_END

using namespace rive;

void URiveViewModelInstance::Initialize(
    rive::ViewModelInstanceRuntime* InViewModelInstance)
{
    ViewModelInstancePtr = InViewModelInstance;
}

void URiveViewModelInstance::BeginDestroy()
{
    ClearCallbacks();
    Properties.Empty();

    Super::BeginDestroy();
}

void URiveViewModelInstance::AddCallbackProperty(
    URiveViewModelInstanceValue* Property)
{
    if (!Property)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModelInstance::AddCallbackProperty() "
                    "Property is null."));

        return;
    }

    CallbackProperties.AddUnique(Property);
}

void URiveViewModelInstance::RemoveCallbackProperty(
    URiveViewModelInstanceValue* Property)
{
    if (!Property)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModelInstance::RemoveCallbackProperty() "
                    "Property is null."));

        return;
    }

    CallbackProperties.Remove(Property);
}

void URiveViewModelInstance::HandleCallbacks()
{
    for (TWeakObjectPtr<URiveViewModelInstanceValue> WeakProperty :
         CallbackProperties)
    {
        if (WeakProperty.IsValid())
        {
            URiveViewModelInstanceValue* Property = WeakProperty.Get();
            if (Property)
            {
                Property->HandleCallbacks();
            }
        }
    }
}

void URiveViewModelInstance::ClearCallbacks()
{
    for (TWeakObjectPtr<URiveViewModelInstanceValue> WeakProperty :
         CallbackProperties)
    {
        if (WeakProperty.IsValid())
        {
            URiveViewModelInstanceValue* Property = WeakProperty.Get();
            if (Property)
            {
                RemoveCallbackProperty(Property);
            }
        }
    }
}

int32 URiveViewModelInstance::GetPropertyCount() const
{
    return ViewModelInstancePtr
               ? static_cast<int32>(ViewModelInstancePtr->propertyCount())
               : 0;
}

template <typename T>
T* URiveViewModelInstance::GetProperty(const FString& PropertyName)
{
    static_assert(TIsDerivedFrom<T, IRiveViewModelPropertyInterface>::Value,
                  "T must implement IRiveViewModelPropertyInterface!");

    FString Key = PropertyName + T::GetSuffix();

    if (UObject** Property = Properties.Find(Key))
    {
        return Cast<T>(*Property);
    }

    if (!ViewModelInstancePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModelInstance::GetProperty() "
                    "ViewModelInstancePtr is null."));

        return nullptr;
    }

    using PropertyType = typename PropertyTypeResolver<T>::Type;

    PropertyType Property = nullptr;

    if constexpr (std::is_same_v<T, URiveViewModelInstanceBoolean>)
    {
        Property = ViewModelInstancePtr->propertyBoolean(
            RiveUtils::ToUTF8(*PropertyName));
    }
    else if constexpr (std::is_same_v<T, URiveViewModelInstanceColor>)
    {
        Property = ViewModelInstancePtr->propertyColor(
            RiveUtils::ToUTF8(*PropertyName));
    }
    else if constexpr (std::is_same_v<T, URiveViewModelInstanceEnum>)
    {
        Property = ViewModelInstancePtr->propertyEnum(
            RiveUtils::ToUTF8(*PropertyName));
    }
    else if constexpr (std::is_same_v<T, URiveViewModelInstanceNumber>)
    {
        Property = ViewModelInstancePtr->propertyNumber(
            RiveUtils::ToUTF8(*PropertyName));
    }
    else if constexpr (std::is_same_v<T, URiveViewModelInstanceString>)
    {
        Property = ViewModelInstancePtr->propertyString(
            RiveUtils::ToUTF8(*PropertyName));
    }
    else if constexpr (std::is_same_v<T, URiveViewModelInstanceTrigger>)
    {
        Property = ViewModelInstancePtr->propertyTrigger(
            RiveUtils::ToUTF8(*PropertyName));
    }
    else if constexpr (std::is_same_v<T, URiveViewModelInstance>)
    {
        Property = ViewModelInstancePtr->propertyViewModel(
            RiveUtils::ToUTF8(*PropertyName));
    }

    if (!Property)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to retrieve property with name '%s'."),
               *Key);

        return nullptr;
    }

    T* PropertyInstance = NewObject<T>(this);
    PropertyInstance->Initialize(Property);

    if constexpr (!std::is_same_v<T, URiveViewModelInstance>)
    {
        PropertyInstance->Initialize(Property);

        PropertyInstance->OnAddCallbackProperty.BindDynamic(
            this,
            &URiveViewModelInstance::AddCallbackProperty);
        PropertyInstance->OnRemoveCallbackProperty.BindDynamic(
            this,
            &URiveViewModelInstance::RemoveCallbackProperty);
    }

    Properties.Add(Key, PropertyInstance);

    return PropertyInstance;
}

URiveViewModelInstanceBoolean* URiveViewModelInstance::GetBooleanProperty(
    const FString& PropertyName)
{
    return GetProperty<URiveViewModelInstanceBoolean>(PropertyName);
}

void URiveViewModelInstance::SetBooleanPropertyValue(
    const FString& PropertyName,
    const bool Value)
{
    URiveViewModelInstanceBoolean* BooleanInstance =
        GetBooleanProperty(PropertyName);

    if (BooleanInstance)
    {
        BooleanInstance->SetValue(Value);
    }
}

bool URiveViewModelInstance::GetBooleanPropertyValue(
    const FString& PropertyName)
{
    URiveViewModelInstanceBoolean* BooleanInstance =
        GetBooleanProperty(PropertyName);

    if (BooleanInstance)
    {
        return BooleanInstance->GetValue();
    }

    return false;
}

URiveViewModelInstanceColor* URiveViewModelInstance::GetColorProperty(
    const FString& PropertyName)
{
    return GetProperty<URiveViewModelInstanceColor>(PropertyName);
}

FColor URiveViewModelInstance::GetColorPropertyValue(
    const FString& PropertyName)
{
    URiveViewModelInstanceColor* ColorInstance = GetColorProperty(PropertyName);

    if (ColorInstance)
    {
        return ColorInstance->GetColor();
    }

    return FColor(0);
}

void URiveViewModelInstance::SetColorPropertyValue(const FString& PropertyName,
                                                   const FColor Value)
{
    URiveViewModelInstanceColor* ColorInstance = GetColorProperty(PropertyName);

    if (ColorInstance)
    {
        ColorInstance->SetColor(Value);
    }
}

URiveViewModelInstanceNumber* URiveViewModelInstance::GetNumberProperty(
    const FString& PropertyName)
{
    return GetProperty<URiveViewModelInstanceNumber>(PropertyName);
}

void URiveViewModelInstance::SetNumberPropertyValue(const FString& PropertyName,
                                                    const float Value)
{
    URiveViewModelInstanceNumber* NumberInstance =
        GetNumberProperty(PropertyName);

    if (NumberInstance)
    {
        NumberInstance->SetValue(Value);
    }
}

float URiveViewModelInstance::GetNumberPropertyValue(
    const FString& PropertyName)
{
    URiveViewModelInstanceNumber* NumberInstance =
        GetNumberProperty(PropertyName);

    if (NumberInstance)
    {
        return NumberInstance->GetValue();
    }

    return 0.0f;
}

URiveViewModelInstanceString* URiveViewModelInstance::GetStringProperty(
    const FString& PropertyName)
{
    return GetProperty<URiveViewModelInstanceString>(PropertyName);
}

void URiveViewModelInstance::SetStringPropertyValue(const FString& PropertyName,
                                                    const FString& Value)
{
    URiveViewModelInstanceString* StringInstance =
        GetStringProperty(PropertyName);

    if (StringInstance)
    {
        StringInstance->SetValue(Value);
    }
}

FString URiveViewModelInstance::GetStringPropertyValue(
    const FString& PropertyName)
{
    URiveViewModelInstanceString* StringInstance =
        GetStringProperty(PropertyName);

    if (StringInstance)
    {
        return StringInstance->GetValue();
    }

    return FString(TEXT(""));
}

URiveViewModelInstanceEnum* URiveViewModelInstance::GetEnumProperty(
    const FString& PropertyName)
{
    return GetProperty<URiveViewModelInstanceEnum>(PropertyName);
}

FString URiveViewModelInstance::GetEnumPropertyValue(
    const FString& PropertyName)
{
    URiveViewModelInstanceEnum* EnumInstance = GetEnumProperty(PropertyName);

    if (EnumInstance)
    {
        return EnumInstance->GetValue();
    }

    return FString(TEXT(""));
}

void URiveViewModelInstance::SetEnumPropertyValue(const FString& PropertyName,
                                                  const FString& Value)
{
    URiveViewModelInstanceEnum* EnumInstance = GetEnumProperty(PropertyName);

    if (EnumInstance)
    {
        EnumInstance->SetValue(Value);
    }
}

TArray<FString> URiveViewModelInstance::GetEnumPropertyValues(
    const FString& PropertyName)
{
    URiveViewModelInstanceEnum* EnumInstance = GetEnumProperty(PropertyName);

    if (EnumInstance)
    {
        return EnumInstance->GetValues();
    }

    return TArray<FString>();
}

URiveViewModelInstanceTrigger* URiveViewModelInstance::GetTriggerProperty(
    const FString& PropertyName)
{
    return GetProperty<URiveViewModelInstanceTrigger>(PropertyName);
}

void URiveViewModelInstance::FireTriggerProperty(const FString& PropertyName)
{
    URiveViewModelInstanceTrigger* TriggerProp =
        GetTriggerProperty(PropertyName);

    if (TriggerProp)
    {
        TriggerProp->Trigger();
    }
}

URiveViewModelInstance* URiveViewModelInstance::GetNestedInstanceByName(
    const FString& PropertyName)
{
    return GetProperty<URiveViewModelInstance>(PropertyName);
}
