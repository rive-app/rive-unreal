#include "Rive/ViewModel/RiveViewModelInstanceEnum.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveUtils.h"

using namespace rive;

FString URiveViewModelInstanceEnum::GetValue()
{
    auto* EnumPtr = GetNativePtr();

    if (!EnumPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceEnum::GetValue() "
                    "GetNativePtr() is null."));

        return FString(TEXT(""));
    }

    return FString(EnumPtr->value().c_str());
}

void URiveViewModelInstanceEnum::SetValue(const FString& Value)
{
    // Since there's a bug in Unreal's GetOptions, we need to
    // validate the value before setting it.

    auto* EnumPtr = GetNativePtr();

    if (!EnumPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceEnum::SetValue() "
                    "GetNativePtr() is null."));

        return;
    }

    if (!GetValues().Contains(Value))
    {
        UE_LOG(LogRive, Error, TEXT("No enum option with name '%s'."), *Value);
        return;
    }

    EnumPtr->value(RiveUtils::ToUTF8(*Value));
}

TArray<FString> URiveViewModelInstanceEnum::GetValues() const
{
    TArray<FString> Values;

    auto* EnumPtr = GetNativePtr();

    if (!EnumPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceEnum::GetValues() "
                    "GetNativePtr() is null."));

        return Values;
    }

    auto EnumValues = EnumPtr->values();
    for (const auto& Val : EnumValues)
    {
        Values.Add(FString(Val.c_str()));
    }

    return Values;
}

void URiveViewModelInstanceEnum::GetValuesInternal(
    TArray<FString>& Options) const
{
    Options = GetValues();
}
