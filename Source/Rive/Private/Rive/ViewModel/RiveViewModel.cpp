#include "Rive/ViewModel/RiveViewModel.h"
#include "Rive/ViewModel/RiveViewModelInstance.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveUtils.h"

THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_runtime.hpp"
THIRD_PARTY_INCLUDES_END

void URiveViewModel::Initialize(rive::ViewModelRuntime* InViewModel)
{
    ViewModelRuntimePtr = InViewModel;
}

FString URiveViewModel::GetName() const
{
    if (!ViewModelRuntimePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::GetName() ViewModelRuntimePtr is null."));

        return FString(TEXT(""));
    }

    return FString(ViewModelRuntimePtr->name().c_str());
}

int32 URiveViewModel::GetInstanceCount() const
{
    if (!ViewModelRuntimePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::GetInstanceCount() "
                    "ViewModelRuntimePtr is null."));

        return 0;
    }

    return static_cast<int32>(ViewModelRuntimePtr->instanceCount());
}

TArray<FString> URiveViewModel::GetInstanceNames() const
{
    TArray<FString> InstanceNames;

    if (!ViewModelRuntimePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::GetInstanceNames() "
                    "ViewModelRuntimePtr is null."));

        return InstanceNames;
    }

    auto Names = ViewModelRuntimePtr->instanceNames();
    for (const auto& Name : Names)
    {
        InstanceNames.Add(FString(Name.c_str()));
    }

    return InstanceNames;
}

TArray<FString> URiveViewModel::GetPropertyNames() const
{
    TArray<FString> PropertyNames;

    if (!ViewModelRuntimePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::GetPropertyNames() "
                    "ViewModelRuntimePtr is null."));

        return PropertyNames;
    }

    auto Properties = ViewModelRuntimePtr->properties();
    for (const auto& Property : Properties)
    {
        PropertyNames.Add(FString(Property.name.c_str()));
    }

    return PropertyNames;
}

rive::DataType URiveViewModel::GetPropertyTypeByName(const FString& Name) const
{
    if (!ViewModelRuntimePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::GetPropertyTypeByName() "
                    "ViewModelRuntimePtr is null."));

        return rive::DataType::none;
    }

    auto Properties = ViewModelRuntimePtr->properties();
    for (auto& Property : Properties)
    {
        if (FString(UTF8_TO_TCHAR(Property.name.c_str())) == Name)
            return Property.type;
    }

    UE_LOG(LogRive,
           Error,
           TEXT("Failed to retrieve Property "
                "with name '%s'."),
           *Name);

    return rive::DataType::none;
}

URiveViewModelInstance* URiveViewModel::CreateWrapperInstance(
    rive::ViewModelInstanceRuntime* RuntimeInstance) const
{
    if (!RuntimeInstance)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::CreateWrapperInstance() "
                    "RuntimeInstance is null."));

        return nullptr;
    }

    URiveViewModelInstance* InstanceWrapper =
        NewObject<URiveViewModelInstance>();
    InstanceWrapper->Initialize(RuntimeInstance);
    return InstanceWrapper;
}

URiveViewModelInstance* URiveViewModel::CreateDefaultInstance() const
{
    if (!ViewModelRuntimePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::CreateDefaultInstance() "
                    "ViewModelRuntimePtr is null."));

        return nullptr;
    }

    return CreateWrapperInstance(ViewModelRuntimePtr->createDefaultInstance());
}

URiveViewModelInstance* URiveViewModel::CreateInstance()
{
    if (!ViewModelRuntimePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::CreateInstance() "
                    "ViewModelRuntimePtr is null."));

        return nullptr;
    }

    return CreateWrapperInstance(ViewModelRuntimePtr->createInstance());
}

URiveViewModelInstance* URiveViewModel::CreateInstanceFromIndex(
    int32 Index) const
{
    if (!ViewModelRuntimePtr)
    {
        UE_LOG(LogRive, Error, TEXT("ViewModelRuntimePtr is null."));
        return nullptr;
    }

    if (Index < 0 ||
        Index >= static_cast<int32>(ViewModelRuntimePtr->instanceCount()))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Index '%d' is out of range. "
                    "Valid range is [0, %d)."),
               Index,
               ViewModelRuntimePtr->instanceCount());
        return nullptr;
    }

    return CreateWrapperInstance(ViewModelRuntimePtr->createInstanceFromIndex(
        static_cast<size_t>(Index)));
}

URiveViewModelInstance* URiveViewModel::CreateInstanceFromName(
    const FString& Name) const
{
    if (!ViewModelRuntimePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveViewModel::CreateInstanceFromName"
                    "ViewModelRuntimePtr is null."));

        return nullptr;
    }

    return CreateWrapperInstance(
        ViewModelRuntimePtr->createInstanceFromName(RiveUtils::ToUTF8(*Name)));
}
