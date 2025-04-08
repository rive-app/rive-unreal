#include "Rive/ViewModel/RiveViewModelInstanceNumber.h"

// Use the Rive namespace for convenience
using namespace rive;

float URiveViewModelInstanceNumber::GetValue()
{
    auto* NumberPtr = GetNativePtr();

    if (!NumberPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceNumber::GetValue() "
                    "GetNativePtr() is null."));

        return 0.0f;
    }

    return NumberPtr->value();
}

void URiveViewModelInstanceNumber::SetValue(float Value)
{
    auto* NumberPtr = GetNativePtr();

    if (!NumberPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceNumber::SetValue() "
                    "GetNativePtr() is null."));

        return;
    }

    NumberPtr->value(Value);
}
