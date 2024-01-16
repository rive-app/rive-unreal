// Copyright Rive, Inc. All rights reserved.

#include "Core/URStateMachine.h"
#include "Logs/RiveCoreLog.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/animation/state_machine_input_instance.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#if WITH_RIVE

UE::Rive::Core::FURStateMachine::FURStateMachine(rive::ArtboardInstance* InNativeArtboardInst)
{
    NativeStateMachinePtr = InNativeArtboardInst->defaultStateMachine();

    if (NativeStateMachinePtr == nullptr)
    {
        NativeStateMachinePtr = InNativeArtboardInst->stateMachineAt(0);
    }
}

UE_DISABLE_OPTIMIZATION

bool UE::Rive::Core::FURStateMachine::Advance(float InSeconds)
{
    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->advanceAndApply(InSeconds);
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not advance state machine as we have detected an empty state machine."));

    return false;
}

uint32 UE::Rive::Core::FURStateMachine::GetInputCount() const
{
    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->inputCount();
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not get input count as we have detected an empty state machine."));

    return 0;
}

rive::SMIInput* UE::Rive::Core::FURStateMachine::GetInput(uint32 AtIndex) const
{
    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->input(AtIndex);
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not get input count as we have detected an empty state machine."));

    return nullptr;
}

void UE::Rive::Core::FURStateMachine::FireTrigger(const FString& InPropertyName) const
{
    if (NativeStateMachinePtr)
    {
        if (rive::SMITrigger* TriggerToBeFired = NativeStateMachinePtr->getTrigger(TCHAR_TO_UTF8(*InPropertyName)))
        {
            TriggerToBeFired->fire();
        }
        else
        {
            UE_LOG(LogRiveCore, Error, TEXT("Could not fire the trigger with given name '%s' as we could not find it."), *InPropertyName);
        }

        return;
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not fire the trigger with given name '%s' as we have detected an empty state machine."), *InPropertyName);
}

bool UE::Rive::Core::FURStateMachine::GetBoolValue(const FString& InPropertyName) const
{
    if (NativeStateMachinePtr)
    {
        if (const rive::SMIBool* BoolProperty = NativeStateMachinePtr->getBool(TCHAR_TO_UTF8(*InPropertyName)))
        {
            return BoolProperty->value();
        }
        
        UE_LOG(LogRiveCore, Error, TEXT("Could not get the boolean property with given name '%s' as we could not find it."), *InPropertyName);

        return false;
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not get the boolean property with given name '%s' as we have detected an empty state machine."), *InPropertyName);

    return false;
}

float UE::Rive::Core::FURStateMachine::GetNumberValue(const FString& InPropertyName) const
{
    if (NativeStateMachinePtr)
    {
        if (const rive::SMINumber* NumberProperty = NativeStateMachinePtr->getNumber(TCHAR_TO_UTF8(*InPropertyName)))
        {
            return NumberProperty->value();
        }

        UE_LOG(LogRiveCore, Error, TEXT("Could not get the number property with given name '%s' as we could not find it."), *InPropertyName);

        return 0;
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not get the number property with given name '%s' as we have detected an empty state machine."), *InPropertyName);

    return 0;
}

void UE::Rive::Core::FURStateMachine::SetBoolValue(const FString& InPropertyName, bool bNewValue)
{
    if (NativeStateMachinePtr)
    {
        if (rive::SMIBool* BoolProperty = NativeStateMachinePtr->getBool(TCHAR_TO_UTF8(*InPropertyName)))
        {
            BoolProperty->value(bNewValue);
        }
        else
        {
            UE_LOG(LogRiveCore, Error, TEXT("Could not get the boolean property with given name '%s' as we could not find it."), *InPropertyName);
        }

        return;
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not get the boolean property with given name '%s' as we have detected an empty state machine."), *InPropertyName);
}

void UE::Rive::Core::FURStateMachine::SetNumberValue(const FString& InPropertyName, float NewValue)
{
    if (NativeStateMachinePtr)
    {
        if (rive::SMINumber* NumberProperty = NativeStateMachinePtr->getNumber(TCHAR_TO_UTF8(*InPropertyName)))
        {
            NumberProperty->value(NewValue);
        }
        else
        {
            UE_LOG(LogRiveCore, Error, TEXT("Could not get the number property with given name '%s' as we could not find it."), *InPropertyName);
        }

        return;
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not get the number property with given name '%s' as we have detected an empty state machine."), *InPropertyName);
}

bool UE::Rive::Core::FURStateMachine::OnMouseButtonDown(const FVector2f& NewPosition)
{
    if (NativeStateMachinePtr)
    {
        NativeStateMachinePtr->pointerDown({ NewPosition.X, NewPosition.Y });

        return true;
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not update state machine with mouse button down event as we have detected an empty state machine."));

    return false;
}

bool UE::Rive::Core::FURStateMachine::OnMouseMove(const FVector2f& NewPosition)
{
    if (NativeStateMachinePtr)
    {
        NativeStateMachinePtr->pointerMove({ NewPosition.X, NewPosition.Y });

        return true;
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not update state machine with mouse move event as we have detected an empty state machine."));

    return false;
}

bool UE::Rive::Core::FURStateMachine::OnMouseButtonUp(const FVector2f& NewPosition)
{
    if (NativeStateMachinePtr)
    {
        NativeStateMachinePtr->pointerUp({ NewPosition.X, NewPosition.Y });

        return true;
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not update state machine with mouse button up event as we have detected an empty state machine."));

    return false;
}

const TArray<UE::Rive::Core::FUREventPtr>& UE::Rive::Core::FURStateMachine::GetReportedEvents()
{
    PopulateReportedEvents();

    return ReportedEvents;
}

bool UE::Rive::Core::FURStateMachine::HasAnyReportedEvents() const
{
    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->reportedEventCount() != 0;
    }
 
    UE_LOG(LogRiveCore, Warning, TEXT("Could not check for reported event(s) as we have detected an empty state machine."));

    return false;
}

void UE::Rive::Core::FURStateMachine::PopulateReportedEvents()
{
    if (HasAnyReportedEvents()) // Only populate when we have atleast one reported event.
    {
        if (NativeStateMachinePtr)
        {
            const size_t NumReportedEvents = NativeStateMachinePtr->reportedEventCount();

            ReportedEvents.Reset(NumReportedEvents);

            for (size_t EventIndex = 0; EventIndex < NumReportedEvents; ++EventIndex)
            {
                ReportedEvents[EventIndex] = MakeUnique<FUREvent>(NativeStateMachinePtr->reportedEventAt(EventIndex));
            }
        }
        else
        {
            UE_LOG(LogRiveCore, Warning, TEXT("Could not get reported event(s) as we have detected an empty state machine."));
        }
    }
    else
    {
        ReportedEvents.Empty();
    }
}

UE_ENABLE_OPTIMIZATION

#endif // WITH_RIVE
