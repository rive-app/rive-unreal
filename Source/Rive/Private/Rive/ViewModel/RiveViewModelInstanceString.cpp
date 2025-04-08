#include "Rive/ViewModel/RiveViewModelInstanceString.h"
#include "Rive/RiveUtils.h"

/**
 * Wrapper class for rive::ViewModelInstanceStringRuntime
 */
FString URiveViewModelInstanceString::GetValue()
{
    auto* StringPtr = GetNativePtr();

    if (!StringPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceString::GetValue() "
                    "GetNativePtr() is null."));

        return FString();
    }

    return FString(StringPtr->value().c_str());
}

void URiveViewModelInstanceString::SetValue(const FString& Value)
{
    auto* StringPtr = GetNativePtr();

    if (!StringPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceString::SetValue() "
                    "GetNativePtr() is null."));
    }

    StringPtr->value(RiveUtils::ToUTF8(*Value));
}
