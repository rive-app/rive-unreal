// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveStateMachine.h"
#include "Rive/ViewModel/RiveViewModelInstance.h"
#include "rive/viewmodel/runtime/viewmodel_instance_runtime.hpp"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "Stats/RiveStats.h"
#include "Rive/RiveUtils.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/generated/animation/state_machine_bool_base.hpp"
#include "rive/generated/animation/state_machine_number_base.hpp"
#include "rive/generated/animation/state_machine_trigger_base.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#if WITH_RIVE

rive::EventReport FRiveStateMachine::NullEvent =
    rive::EventReport(nullptr, 0.f);

FRiveStateMachine::FRiveStateMachine(
    rive::ArtboardInstance* InNativeArtboardInst,
    const FString& InStateMachineName)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        RiveRenderer->GetThreadDataCS().Lock();
    }

    if (InStateMachineName.IsEmpty())
    {
        NativeStateMachinePtr = InNativeArtboardInst->defaultStateMachine();
    }
    else
    {
        NativeStateMachinePtr = InNativeArtboardInst->stateMachineNamed(
            RiveUtils::ToUTF8(*InStateMachineName));
    }

    if (NativeStateMachinePtr == nullptr)
    {
        NativeStateMachinePtr = InNativeArtboardInst->stateMachineAt(0);
    }

    // Note: Even at this point, the state machine can still be empty, because
    // there aren't any state machines in the artboard
    if (NativeStateMachinePtr)
    {
        StateMachineName = NativeStateMachinePtr->name().c_str();

        for (uint32 i = 0; i < GetInputCount(); ++i)
        {
            const rive::SMIInput* Input = GetInput(i);
            if (Input->input()->is<rive::StateMachineBoolBase>())
            {
                BoolInputNames.Add(Input->name().c_str());
            }
            else if (Input->input()->is<rive::StateMachineNumberBase>())
            {
                NumberInputNames.Add(Input->name().c_str());
            }
            else if (Input->input()->is<rive::StateMachineTriggerBase>())
            {
                TriggerInputNames.Add(Input->name().c_str());
            }
            else
            {
                UE_LOG(LogRive,
                       Warning,
                       TEXT("Found input of unknown type '%d' when getting "
                            "inputs from "
                            "StateMachine '%s' from Artboard '%hs'"),
                       Input->inputCoreType(),
                       *GetStateMachineName(),
                       InNativeArtboardInst->name().c_str())
            }
        }
    }

    if (RiveRenderer)
    {
        RiveRenderer->GetThreadDataCS().Unlock();
    }
}

bool FRiveStateMachine::Advance(float InSeconds)
{
    SCOPED_NAMED_EVENT_TEXT(TEXT("FRiveStateMachine::Advance"), FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FRiveStateMachine::Advance"),
                                STAT_STATEMACHINE_ADVANCE,
                                STATGROUP_Rive);
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to Advance the StateMachine as we do not have a "
                    "valid renderer."));
        return false;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->advanceAndApply(InSeconds);
    }

    return false;
}

uint32 FRiveStateMachine::GetInputCount() const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to GetInputCount on the StateMachine as we do not "
                    "have a valid renderer."));
        return 0;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->inputCount();
    }

    return 0;
}

rive::SMIInput* FRiveStateMachine::GetInput(uint32 AtIndex) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to GetInput on the StateMachine as we do not have "
                    "a valid renderer."));
        return nullptr;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->input(AtIndex);
    }

    return nullptr;
}

void FRiveStateMachine::FireTrigger(const FString& InPropertyName) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return;
    }

    if (rive::SMITrigger* TriggerToBeFired = NativeStateMachinePtr->getTrigger(
            RiveUtils::ToUTF8(*InPropertyName)))
    {
        TriggerToBeFired->fire();
        return;
    }

    UE_LOG(LogRive,
           Error,
           TEXT("Could not fire the trigger with given name '%s' as we could "
                "not find it."),
           *InPropertyName);
}

bool FRiveStateMachine::GetBoolValue(const FString& InPropertyName) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to GetBoolValue on the StateMachine as we do not "
                    "have a valid renderer."));
        return false;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return false;
    }

    if (const rive::SMIBool* BoolProperty =
            NativeStateMachinePtr->getBool(RiveUtils::ToUTF8(*InPropertyName)))
    {
        return BoolProperty->value();
    }

    UE_LOG(LogRive,
           Error,
           TEXT("Could not get the boolean property with given name '%s' as "
                "we could not find it."),
           *InPropertyName);
    return false;
}

float FRiveStateMachine::GetNumberValue(const FString& InPropertyName) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to GetNumberValue on the StateMachine as we "
                    "do not have a valid renderer."));
        return 0.f;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return 0;
    }

    if (const rive::SMINumber* NumberProperty =
            NativeStateMachinePtr->getNumber(
                RiveUtils::ToUTF8(*InPropertyName)))
    {
        return NumberProperty->value();
    }

    UE_LOG(LogRive,
           Error,
           TEXT("Could not get the number property with given name '%s' as we "
                "could not find it."),
           *InPropertyName);
    return 0;
}

void FRiveStateMachine::SetBoolValue(const FString& InPropertyName,
                                     bool bNewValue)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to SetBoolValue on the StateMachine as we do not "
                    "have a valid renderer."));
        return;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return;
    }

    if (rive::SMIBool* BoolProperty =
            NativeStateMachinePtr->getBool(RiveUtils::ToUTF8(*InPropertyName)))
    {
        BoolProperty->value(bNewValue);
        return;
    }

    UE_LOG(LogRive,
           Error,
           TEXT("Could not get the boolean property with given name '%s' "
                "as we could not find it."),
           *InPropertyName);
}

void FRiveStateMachine::SetNumberValue(const FString& InPropertyName,
                                       float NewValue)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to SetNumberValue on the StateMachine "
                    "as we do not have a valid renderer."));
        return;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return;
    }

    if (rive::SMINumber* NumberProperty = NativeStateMachinePtr->getNumber(
            RiveUtils::ToUTF8(*InPropertyName)))
    {
        NumberProperty->value(NewValue);
        return;
    }

    UE_LOG(LogRive,
           Error,
           TEXT("Could not get the number property with given name '%s' as we "
                "could not find it."),
           *InPropertyName);
}

bool FRiveStateMachine::PointerDown(const FVector2f& NewPosition)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to call PointerDown on the StateMachine as we do "
                    "not have a valid "
                    "renderer."));
        return false;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return false;
    }

    rive::HitResult HitResult =
        NativeStateMachinePtr->pointerDown({NewPosition.X, NewPosition.Y});
    return HitResult != rive::HitResult::none;
}

bool FRiveStateMachine::PointerMove(const FVector2f& NewPosition)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to call PointerMove on the StateMachine as we do "
                    "not have a valid "
                    "renderer."));
        return false;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return false;
    }

    rive::HitResult HitResult =
        NativeStateMachinePtr->pointerMove({NewPosition.X, NewPosition.Y});
    return HitResult != rive::HitResult::none;
}

bool FRiveStateMachine::PointerUp(const FVector2f& NewPosition)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to call PointerUp on the StateMachine as we do "
                    "not have a valid renderer."));
        return false;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return false;
    }

    rive::HitResult HitResult =
        NativeStateMachinePtr->pointerUp({NewPosition.X, NewPosition.Y});
    return HitResult != rive::HitResult::none;
}

bool FRiveStateMachine::PointerExit(const FVector2f& NewPosition)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to call PointerExit on the StateMachine as we do "
                    "not have a valid renderer."));
        return false;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return false;
    }

    rive::HitResult HitResult =
        NativeStateMachinePtr->pointerExit({NewPosition.X, NewPosition.Y});
    return HitResult != rive::HitResult::none;
}

const rive::EventReport FRiveStateMachine::GetReportedEvent(int32 AtIndex) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to GetReportedEvent on the StateMachine as we do "
                    "not have a valid renderer."));
        return NullEvent;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr || !HasAnyReportedEvents())
    {
        return NullEvent;
    }

    return NativeStateMachinePtr->reportedEventAt(AtIndex);
}

int32 FRiveStateMachine::GetReportedEventsCount() const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to GetReportedEventsCount on the StateMachine as "
                    "we do not have a valid renderer."));
        return 0;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr || !HasAnyReportedEvents())
    {
        return 0;
    }

    return NativeStateMachinePtr->reportedEventCount();
}

bool FRiveStateMachine::HasAnyReportedEvents() const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to HasAnyReportedEvents on the StateMachine as we "
                    "do not have a valid renderer."));
        return false;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return false;
    }

    return NativeStateMachinePtr->reportedEventCount() != 0;
}

void FRiveStateMachine::SetViewModelInstance(
    URiveViewModelInstance* RiveViewModelInstance)
{
    if (!NativeStateMachinePtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("SetViewModelInstance failed: "
                    "NativeStateMachinePtr is null."));
        return;
    }

    if (!RiveViewModelInstance || !RiveViewModelInstance->GetNativePtr())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("SetViewModelInstance failed: "
                    "ViewModelInstance is invalid or null."));
        return;
    }

    // Retrieve the native instance from the wrapper
    rive::ViewModelInstanceRuntime* NativeInstance =
        RiveViewModelInstance->GetNativePtr();

    // Set the data context on the native Rive state machine
    NativeStateMachinePtr->bindViewModelInstance(NativeInstance->instance());
}

#endif // WITH_RIVE
