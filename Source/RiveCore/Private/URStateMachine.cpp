// Copyright Rive, Inc. All rights reserved.

#include "URStateMachine.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveCoreLog.h"

#if WITH_RIVE
#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/animation/state_machine_input_instance.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#if WITH_RIVE

rive::EventReport UE::Rive::Core::FURStateMachine::NullEvent = rive::EventReport(nullptr, 0.f);

UE::Rive::Core::FURStateMachine::FURStateMachine(rive::ArtboardInstance* InNativeArtboardInst, const FString& InStateMachineName)
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
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
        NativeStateMachinePtr = InNativeArtboardInst->stateMachineNamed(TCHAR_TO_UTF8(*InStateMachineName));
    }
    
    if (NativeStateMachinePtr == nullptr)
    {
        NativeStateMachinePtr = InNativeArtboardInst->stateMachineAt(0);
    }

    // Note: Even at this point, the state machine can still be empty, because there aren't any state machines in the artboard
    if (NativeStateMachinePtr)
    {
        StateMachineName = NativeStateMachinePtr->name().c_str();
    }

    if (RiveRenderer)
    {
        RiveRenderer->GetThreadDataCS().Unlock();
    }
}

bool UE::Rive::Core::FURStateMachine::Advance(float InSeconds)
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to Advance the StateMachine as we do not have a valid renderer."));
        return false;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->advanceAndApply(InSeconds);
    }
    
    return false;
}

uint32 UE::Rive::Core::FURStateMachine::GetInputCount() const
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to GetInputCount on the StateMachine as we do not have a valid renderer."));
        return 0;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->inputCount();
    }
    
    return 0;
}

rive::SMIInput* UE::Rive::Core::FURStateMachine::GetInput(uint32 AtIndex) const
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to GetInput on the StateMachine as we do not have a valid renderer."));
        return nullptr;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (NativeStateMachinePtr)
    {
        return NativeStateMachinePtr->input(AtIndex);
    }
    
    return nullptr;
}

void UE::Rive::Core::FURStateMachine::FireTrigger(const FString& InPropertyName) const
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr)
    {
        return;
    }
    
    if (rive::SMITrigger* TriggerToBeFired = NativeStateMachinePtr->getTrigger(TCHAR_TO_UTF8(*InPropertyName)))
    {
        TriggerToBeFired->fire();
        return;
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not fire the trigger with given name '%s' as we could not find it."), *InPropertyName);
}

bool UE::Rive::Core::FURStateMachine::GetBoolValue(const FString& InPropertyName) const
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to GetBoolValue on the StateMachine as we do not have a valid renderer."));
        return false;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr)
    {
        return false;
    }
    
    if (const rive::SMIBool* BoolProperty = NativeStateMachinePtr->getBool(TCHAR_TO_UTF8(*InPropertyName)))
    {
        return BoolProperty->value();
    }

    UE_LOG(LogRiveCore, Error, TEXT("Could not get the boolean property with given name '%s' as we could not find it."), *InPropertyName);
    return false;
}

float UE::Rive::Core::FURStateMachine::GetNumberValue(const FString& InPropertyName) const
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to GetNumberValue on the StateMachine as we do not have a valid renderer."));
        return 0.f;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr)
    {
        return 0;
    }
    
    if (const rive::SMINumber* NumberProperty = NativeStateMachinePtr->getNumber(TCHAR_TO_UTF8(*InPropertyName)))
    {
        return NumberProperty->value();
    }
    
    UE_LOG(LogRiveCore, Error, TEXT("Could not get the number property with given name '%s' as we could not find it."), *InPropertyName);
    return 0;
}

void UE::Rive::Core::FURStateMachine::SetBoolValue(const FString& InPropertyName, bool bNewValue)
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to SetBoolValue on the StateMachine as we do not have a valid renderer."));
        return;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr)
    {
        return;
    }
    
    if (rive::SMIBool* BoolProperty = NativeStateMachinePtr->getBool(TCHAR_TO_UTF8(*InPropertyName)))
    {
        BoolProperty->value(bNewValue);
        return;
    }
    
    UE_LOG(LogRiveCore, Error, TEXT("Could not get the boolean property with given name '%s' as we could not find it."), *InPropertyName);
}

void UE::Rive::Core::FURStateMachine::SetNumberValue(const FString& InPropertyName, float NewValue)
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to SetNumberValue on the StateMachine as we do not have a valid renderer."));
        return;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr)
    {
        return;
    }
    
    if (rive::SMINumber* NumberProperty = NativeStateMachinePtr->getNumber(TCHAR_TO_UTF8(*InPropertyName)))
    {
        NumberProperty->value(NewValue);
        return;
    }
 
    UE_LOG(LogRiveCore, Error, TEXT("Could not get the number property with given name '%s' as we could not find it."), *InPropertyName);
}

bool UE::Rive::Core::FURStateMachine::OnMouseButtonDown(const FVector2f& NewPosition)
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to call OnMouseButtonDown on the StateMachine as we do not have a valid renderer."));
        return false;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeStateMachinePtr)
    {
        return false;
    }
    
    NativeStateMachinePtr->pointerDown({ NewPosition.X, NewPosition.Y });
    return true;
}

bool UE::Rive::Core::FURStateMachine::OnMouseMove(const FVector2f& NewPosition)
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to call OnMouseMove on the StateMachine as we do not have a valid renderer."));
        return false;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr)
    {
        return false;
    }
    
    NativeStateMachinePtr->pointerMove({ NewPosition.X, NewPosition.Y });
    return true;
}

bool UE::Rive::Core::FURStateMachine::OnMouseButtonUp(const FVector2f& NewPosition)
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to call OnMouseButtonUp on the StateMachine as we do not have a valid renderer."));
        return false;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr)
    {
        return false;
    }
   
    NativeStateMachinePtr->pointerUp({ NewPosition.X, NewPosition.Y });
    return true;
}

const rive::EventReport UE::Rive::Core::FURStateMachine::GetReportedEvent(int32 AtIndex) const
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to GetReportedEvent on the StateMachine as we do not have a valid renderer."));
        return NullEvent;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr || !HasAnyReportedEvents())
    {
        return NullEvent;
    }

    return NativeStateMachinePtr->reportedEventAt(AtIndex);
}

int32 UE::Rive::Core::FURStateMachine::GetReportedEventsCount() const
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to GetReportedEventsCount on the StateMachine as we do not have a valid renderer."));
        return 0;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr || !HasAnyReportedEvents())
    {
        return 0;
    }
    
    return NativeStateMachinePtr->reportedEventCount();
}

bool UE::Rive::Core::FURStateMachine::HasAnyReportedEvents() const
{
    Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRiveCore, Error, TEXT("Failed to HasAnyReportedEvents on the StateMachine as we do not have a valid renderer."));
        return false;
    }
    
    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
    
    if (!NativeStateMachinePtr)
    {
        return false;
    }
    
    return NativeStateMachinePtr->reportedEventCount() != 0;
}


#endif // WITH_RIVE
