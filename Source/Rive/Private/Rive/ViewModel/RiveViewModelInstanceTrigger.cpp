#include "Rive/ViewModel/RiveViewModelInstanceTrigger.h"

// Use the Rive namespace for convenience
using namespace rive;

void URiveViewModelInstanceTrigger::Trigger()
{
    auto* TriggerPtr = GetNativePtr();

    if (!TriggerPtr)
    {
        UE_LOG(LogTemp,
               Error,
               TEXT("URiveViewModelInstanceTrigger::Trigger() "
                    "GetNativePtr() is null."));

        return;
    }

    TriggerPtr->trigger();
    OnValueChangedMulti.Broadcast();
}
