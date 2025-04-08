#include "Rive/ViewModel/RiveViewModelInstanceColor.h"

// Use the Rive namespace for convenience
using namespace rive;

FColor URiveViewModelInstanceColor::GetColor()
{
    auto* ColorPtr = GetNativePtr();

    if (!ColorPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceColor::GetColor() "
                    "GetNativePtr() is null."));

        return FColor(0);
    }

    return FColor(ColorPtr->value());
}

void URiveViewModelInstanceColor::SetColor(FColor Color)
{
    auto* ColorPtr = GetNativePtr();

    if (!ColorPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceColor::SetColor() "
                    "GetNativePtr() is null."));

        return;
    }

    ColorPtr->value(Color.ToPackedARGB());
}
