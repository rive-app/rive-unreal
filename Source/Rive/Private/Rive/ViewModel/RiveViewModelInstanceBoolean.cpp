#include "Rive/ViewModel/RiveViewModelInstanceBoolean.h"

// Use the Rive namespace for convenience
using namespace rive;

bool URiveViewModelInstanceBoolean::GetValue()
{
    auto* BooleanPtr = GetNativePtr();

    if (!BooleanPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceBoolean::GetValue() "
                    "GetNativePtr() is null."));

        return false;
    }

    return BooleanPtr->value();
}

void URiveViewModelInstanceBoolean::SetValue(bool Value)
{
    auto* BooleanPtr = GetNativePtr();

    if (!BooleanPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceBoolean::SetValue() "
                    "GetNativePtr() is null."));

        return;
    }

    BooleanPtr->value(Value);
}
