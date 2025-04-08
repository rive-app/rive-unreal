// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveArtboard.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveEvent.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveStateMachine.h"
#include "Rive/ViewModel/RiveViewModelInstance.h"
#include "Stats/RiveStats.h"
#include "Rive/RiveUtils.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/viewmodel/runtime/viewmodel_instance_runtime.hpp"
#include "rive/animation/state_machine_input.hpp"
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/generated/animation/state_machine_bool_base.hpp"
#include "rive/generated/animation/state_machine_number_base.hpp"
#include "rive/generated/animation/state_machine_trigger_base.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#if WITH_RIVE

void URiveArtboard::BeginDestroy()
{
    Deinitialize();
    UObject::BeginDestroy();
}

void URiveArtboard::AdvanceStateMachine(float InDeltaSeconds)
{
    if (!RiveRenderTarget)
        return;

    FRiveStateMachine* StateMachine = GetStateMachine();
    if (StateMachine && StateMachine->IsValid())
    {
        if (!bIsReceivingInput)
        {
            if (StateMachine->HasAnyReportedEvents())
            {
                PopulateReportedEvents();
            }
            StateMachine->Advance(InDeltaSeconds);
        }
    }

    if (CurrentViewModelInstance.IsValid())
        CurrentViewModelInstance->HandleCallbacks();
}

void URiveArtboard::Transform(const FVector2f& One,
                              const FVector2f& Two,
                              const FVector2f& T)
{
    if (!RiveRenderTarget)
    {
        return;
    }

    RiveRenderTarget->Transform(One.X, One.Y, Two.X, Two.Y, T.X, T.Y);
}

void URiveArtboard::Translate(const FVector2f& InVector)
{
    if (!RiveRenderTarget)
    {
        return;
    }

    RiveRenderTarget->Translate(InVector);
}

void URiveArtboard::Align(const FBox2f InBox,
                          ERiveFitType InFitType,
                          ERiveAlignment InAlignment,
                          float InScaleFactor)
{
    if (!RiveRenderTarget)
    {
        return;
    }
    RiveRenderTarget->Align(InBox,
                            InFitType,
                            FRiveAlignment::GetAlignment(InAlignment),
                            InScaleFactor,
                            GetNativeArtboard());
}

void URiveArtboard::Align(ERiveFitType InFitType,
                          ERiveAlignment InAlignment,
                          float InScaleFactor)
{
    if (!RiveRenderTarget)
    {
        return;
    }
    RiveRenderTarget->Align(InFitType,
                            FRiveAlignment::GetAlignment(InAlignment),
                            InScaleFactor,
                            GetNativeArtboard());
}

FMatrix URiveArtboard::GetTransformMatrix() const
{
    if (!RiveRenderTarget)
    {
        return {};
    }
    return RiveRenderTarget->GetTransformMatrix();
}

void URiveArtboard::Draw()
{
    if (!RiveRenderTarget)
    {
        return;
    }

    RiveRenderTarget->Draw(GetNativeArtboard());
    LastDrawTransform = GetTransformMatrix();
}

void URiveArtboard::FireTrigger(const FString& InPropertyName) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
        if (const FRiveStateMachine* StateMachine = GetStateMachine())
        {
            StateMachine->FireTrigger(InPropertyName);
        }
    }
}

void URiveArtboard::FireTriggerAtPath(const FString& InInputName,
                                      const FString& InPath) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

        if (!NativeArtboardPtr)
        {
            UE_LOG(LogRive, Warning, TEXT("Invalid Artboard Pointer."));
            return;
        }

        rive::SMITrigger* SmiTrigger =
            NativeArtboardPtr->getTrigger(RiveUtils::ToUTF8(*InInputName),
                                          RiveUtils::ToUTF8(*InPath));
        if (!SmiTrigger)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Invalid input for %s at path %s"),
                   *InInputName,
                   *InPath);
            return;
        }

        if (!SmiTrigger->input()->is<rive::StateMachineTriggerBase>())
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Input for %s at path %s is not a trigger"),
                   *InInputName,
                   *InPath);
            return;
        }

        SmiTrigger->fire();
    }
}

bool URiveArtboard::GetBoolValue(const FString& InPropertyName) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
        if (const FRiveStateMachine* StateMachine = GetStateMachine())
        {
            return StateMachine->GetBoolValue(InPropertyName);
        }
    }
    return false;
}

bool URiveArtboard::GetBoolValueAtPath(const FString& InInputName,
                                       const FString& InPath,
                                       bool& OutSuccess) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

        if (!NativeArtboardPtr)
        {
            UE_LOG(LogRive, Warning, TEXT("Invalid Artboard Pointer."));
            OutSuccess = false;
            return false;
        }
        rive::SMIBool* SmiBool =
            NativeArtboardPtr->getBool(RiveUtils::ToUTF8(*InInputName),
                                       RiveUtils::ToUTF8(*InPath));
        if (!SmiBool)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Invalid input for %s at path %s"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return false;
        }

        if (!SmiBool->input()->is<rive::StateMachineBoolBase>())
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Input for %s at path %s is not a bool"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return false;
        }

        OutSuccess = true;
        return SmiBool->value();
    }

    OutSuccess = false;
    return false;
}

float URiveArtboard::GetNumberValue(const FString& InPropertyName) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
        if (const FRiveStateMachine* StateMachine = GetStateMachine())
        {
            return StateMachine->GetNumberValue(InPropertyName);
        }
    }
    return 0.f;
}

float URiveArtboard::GetNumberValueAtPath(const FString& InInputName,
                                          const FString& InPath,
                                          bool& OutSuccess) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

        if (!NativeArtboardPtr)
        {
            UE_LOG(LogRive, Warning, TEXT("Invalid Artboard Pointer."));
            OutSuccess = false;
            return 0.f;
        }

        rive::SMINumber* SmiNumber =
            NativeArtboardPtr->getNumber(RiveUtils::ToUTF8(*InInputName),
                                         RiveUtils::ToUTF8(*InPath));
        if (!SmiNumber)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Invalid input for %s at path %s"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return 0.f;
        }

        if (!SmiNumber->input()->is<rive::StateMachineNumberBase>())
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Input for %s at path %s is not a number"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return 0.f;
        }

        OutSuccess = true;
        return SmiNumber->value();
    }

    OutSuccess = false;
    return 0.f;
}

FString URiveArtboard::GetTextValue(const FString& InPropertyName) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
        if (const FRiveStateMachine* StateMachine = GetStateMachine())
        {
            if (const rive::TextValueRunBase* TextValueRun =
                    NativeArtboardPtr->find<rive::TextValueRunBase>(
                        RiveUtils::ToUTF8(*InPropertyName)))
            {
                return FString{TextValueRun->text().c_str()};
            }
        }
    }
    return {};
}

FString URiveArtboard::GetTextValueAtPath(const FString& InInputName,
                                          const FString& InPath,
                                          bool& OutSuccess) const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

        if (!NativeArtboardPtr)
        {
            UE_LOG(LogRive, Warning, TEXT("Invalid Artboard Pointer."));
            OutSuccess = false;
            return {};
        }

        rive::TextValueRunBase* TextValueRun =
            NativeArtboardPtr->getTextRun(RiveUtils::ToUTF8(*InInputName),
                                          RiveUtils::ToUTF8(*InPath));
        if (!TextValueRun)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Invalid input for %s at path %s"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return {};
        }

        OutSuccess = true;
        return {TextValueRun->text().c_str()};
    }

    OutSuccess = false;
    return {};
}

void URiveArtboard::SetBoolValue(const FString& InPropertyName, bool bNewValue)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
        if (FRiveStateMachine* StateMachine = GetStateMachine())
        {
            StateMachine->SetBoolValue(InPropertyName, bNewValue);
        }
    }
}

void URiveArtboard::SetBoolValueAtPath(const FString& InInputName,
                                       bool InValue,
                                       const FString& InPath,
                                       bool& OutSuccess)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

        if (!NativeArtboardPtr)
        {
            UE_LOG(LogRive, Warning, TEXT("Invalid Artboard Pointer."));
            OutSuccess = false;
            return;
        }

        rive::SMIBool* SmiBool =
            NativeArtboardPtr->getBool(RiveUtils::ToUTF8(*InInputName),
                                       RiveUtils::ToUTF8(*InPath));
        if (!SmiBool)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Invalid input for %s at path %s"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return;
        }

        if (!SmiBool->input()->is<rive::StateMachineBoolBase>())
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Input for %s at path %s is not a bool"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return;
        }

        SmiBool->value(InValue);
        OutSuccess = true;
    }

    OutSuccess = false;
}

void URiveArtboard::SetNumberValue(const FString& InPropertyName,
                                   float NewValue)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
        if (FRiveStateMachine* StateMachine = GetStateMachine())
        {
            StateMachine->SetNumberValue(InPropertyName, NewValue);
        }
    }
}

void URiveArtboard::SetNumberValueAtPath(const FString& InInputName,
                                         float InValue,
                                         const FString& InPath,
                                         bool& OutSuccess)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

        if (!NativeArtboardPtr)
        {
            UE_LOG(LogRive, Warning, TEXT("Invalid Artboard Pointer."));
            OutSuccess = false;
            return;
        }

        rive::SMINumber* SmiNumber =
            NativeArtboardPtr->getNumber(RiveUtils::ToUTF8(*InInputName),
                                         RiveUtils::ToUTF8(*InPath));
        if (!SmiNumber)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Invalid input for %s at path %s"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return;
        }

        if (!SmiNumber->input()->is<rive::StateMachineNumberBase>())
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Input for %s at path %s is not a number"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return;
        }

        SmiNumber->value(InValue);
        OutSuccess = true;
    }

    OutSuccess = false;
}

void URiveArtboard::SetTextValue(const FString& InPropertyName,
                                 const FString& NewValue)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
        if (const FRiveStateMachine* StateMachine = GetStateMachine())
        {
            if (rive::TextValueRunBase* TextValueRun =
                    NativeArtboardPtr->find<rive::TextValueRunBase>(
                        RiveUtils::ToUTF8(*InPropertyName)))
            {
                TextValueRun->text(RiveUtils::ToUTF8(*NewValue));
            }
        }
    }
}

void URiveArtboard::SetTextValueAtPath(const FString& InInputName,
                                       const FString& InValue,
                                       const FString& InPath,
                                       bool& OutSuccess)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

        if (!NativeArtboardPtr)
        {
            UE_LOG(LogRive, Warning, TEXT("Invalid Artboard Pointer."));
            OutSuccess = false;
            return;
        }

        rive::TextValueRunBase* TextValueRun =
            NativeArtboardPtr->getTextRun(RiveUtils::ToUTF8(*InInputName),
                                          RiveUtils::ToUTF8(*InPath));
        if (!TextValueRun)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Invalid input for %s at path %s"),
                   *InInputName,
                   *InPath);
            OutSuccess = false;
            return;
        }

        TextValueRun->text(RiveUtils::ToUTF8(*InValue));
        OutSuccess = true;
    }

    OutSuccess = false;
}

bool URiveArtboard::BindNamedRiveEvent(const FString& EventName,
                                       const FRiveNamedEventDelegate& Event)
{
    if (EventNames.Contains(EventName))
    {
        NamedRiveEventsDelegates.FindOrAdd(EventName, {}).AddUnique(Event);
        return true;
    }
    UE_LOG(LogRive,
           Warning,
           TEXT("Unable to bind event '%s' to Artboard '%s' as the event does "
                "not exist"),
           *EventName,
           *GetArtboardName())
    return false;
}

bool URiveArtboard::UnbindNamedRiveEvent(const FString& EventName,
                                         const FRiveNamedEventDelegate& Event)
{
    if (EventNames.Contains(EventName))
    {
        if (FRiveNamedEventsDelegate* NamedRiveDelegate =
                NamedRiveEventsDelegates.Find(EventName))
        {
            NamedRiveDelegate->Remove(Event);
            if (!NamedRiveDelegate->IsBound())
            {
                NamedRiveEventsDelegates.Remove(EventName);
            }
        }
        return true;
    }
    UE_LOG(LogRive,
           Warning,
           TEXT("Unable to bind event '%s' to Artboard '%s' as the event does "
                "not exist"),
           *EventName,
           *GetArtboardName())
    return false;
}

bool URiveArtboard::TriggerNamedRiveEvent(const FString& EventName,
                                          float ReportedDelaySeconds)
{
    if (NativeArtboardPtr && GetStateMachine())
    {
        if (rive::Component* Component =
                NativeArtboardPtr->find(RiveUtils::ToUTF8(*EventName)))
        {
            if (Component->is<rive::Event>())
            {
                rive::Event* Event = Component->as<rive::Event>();
                const rive::CallbackData CallbackData(
                    GetStateMachine()->GetNativeStateMachinePtr().get(),
                    ReportedDelaySeconds);
                Event->trigger(CallbackData);
                UE_LOG(LogRive,
                       Warning,
                       TEXT("TRIGGERED event '%s' for Artboard '%s'"),
                       *EventName,
                       *GetArtboardName())
                return true;
            }
        }
        UE_LOG(LogRive,
               Warning,
               TEXT("Unable to trigger event '%s' for Artboard '%s' as the "
                    "Artboard is not ready "
                    "or it doesn't have a state machine"),
               *EventName,
               *GetArtboardName())
    }
    else
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("Unable to trigger event '%s' for Artboard '%s' as the "
                    "event does not exist"),
               *EventName,
               *GetArtboardName())
    }

    return false;
}

void URiveArtboard::PointerDown(const FVector2f& NewPosition)
{
    FRiveStateMachine* StateMachine = GetStateMachine();
    if (StateMachine)
    {
        StateMachine->PointerDown(NewPosition);
    }
}

void URiveArtboard::PointerUp(const FVector2f& NewPosition)
{
    FRiveStateMachine* StateMachine = GetStateMachine();
    if (StateMachine)
    {
        StateMachine->PointerUp(NewPosition);
    }
}

void URiveArtboard::PointerMove(const FVector2f& NewPosition)
{
    FRiveStateMachine* StateMachine = GetStateMachine();
    if (StateMachine)
    {
        StateMachine->PointerMove(NewPosition);
    }
}

void URiveArtboard::PointerExit(const FVector2f& NewPosition)
{
    FRiveStateMachine* StateMachine = GetStateMachine();
    if (StateMachine)
    {
        StateMachine->PointerExit(NewPosition);
    }
}

FVector2f URiveArtboard::GetLocalCoordinate(const FVector2f& InPosition,
                                            const FIntPoint& InTextureSize,
                                            ERiveAlignment InAlignment,
                                            ERiveFitType InFitType) const
{
    FVector2f Alignment = FRiveAlignment::GetAlignment(InAlignment);
    rive::Mat2D Transform = rive::computeAlignment(
        static_cast<rive::Fit>(InFitType),
        rive::Alignment(Alignment.X, Alignment.Y),
        rive::AABB(0.f, 0.f, InTextureSize.X, InTextureSize.Y),
        NativeArtboardPtr->bounds());

    rive::Vec2D Vector =
        Transform.invertOrIdentity() * rive::Vec2D(InPosition.X, InPosition.Y);
    return {Vector.x, Vector.y};
}

FVector2f URiveArtboard::GetLocalCoordinatesFromExtents(
    const FVector2f& InPosition,
    const FBox2f& InExtents,
    const FIntPoint& TextureSize,
    ERiveAlignment Alignment,
    ERiveFitType InFitType) const
{
    const FVector2f RelativePosition = InPosition - InExtents.Min;
    const FVector2f Ratio{
        TextureSize.X / InExtents.GetSize().X,
        TextureSize.Y /
            InExtents.GetSize().Y}; // Ratio should be the same for X and Y
    const FVector2f TextureRelativePosition = RelativePosition * Ratio;

    return GetLocalCoordinate(TextureRelativePosition,
                              TextureSize,
                              Alignment,
                              InFitType);
}

void URiveArtboard::Initialize(
    URiveFile* InRiveFile,
    const TSharedPtr<IRiveRenderTarget>& InRiveRenderTarget)
{
    Initialize(InRiveFile, InRiveRenderTarget, 0, "");
}

void URiveArtboard::Initialize(URiveFile* InRiveFile,
                               TSharedPtr<IRiveRenderTarget> InRiveRenderTarget,
                               int32 InIndex,
                               const FString& InStateMachineName)
{
    RiveRenderTarget = InRiveRenderTarget;
    StateMachineName = InStateMachineName;
    ArtboardIndex = InIndex;
    RiveFile = InRiveFile;

    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to Initialize the Artboard as we do not have a "
                    "valid renderer."));
        return;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!RiveFile.IsValid() || !RiveFile->GetNativeFile())
    {
        return;
    }

    // If we have a RiveFile reference, it means we've started up before. Make
    // sure if the RiveFile is initializing, we deinitialize
    if (!RiveFileDeinitializationHandle.IsValid())
    {
        RiveFileDeinitializationHandle =
            RiveFile->OnStartInitializingDelegate.AddUObject(
                this,
                &URiveArtboard::Deinitialize);
    }

    if (!RiveFileReinitializationHandle.IsValid())
    {
        RiveFileReinitializationHandle =
            RiveFile->OnInitializedDelegate.AddUObject(
                this,
                &URiveArtboard::Reinitialize);
    }

    int32 Index = InIndex;
    if (Index >= RiveFile->GetNativeFile()->artboardCount())
    {
        Index = RiveFile->GetNativeFile()->artboardCount() - 1;
        UE_LOG(LogRive,
               Warning,
               TEXT("Artboard index specified is out of bounds, using the last "
                    "available artboard "
                    "index instead, which is %d"),
               Index);
    }

    if (const rive::Artboard* NativeArtboard =
            RiveFile->GetNativeFile()->artboard(Index))
    {
        Initialize_Internal(NativeArtboard);
    }
}

void URiveArtboard::Initialize(URiveFile* InRiveFile,
                               TSharedPtr<IRiveRenderTarget> InRiveRenderTarget,
                               const FString& InName,
                               const FString& InStateMachineName)
{
    RiveRenderTarget = InRiveRenderTarget;
    StateMachineName = InStateMachineName;
    RiveFile = InRiveFile;

    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to Initialize the Artboard as we do not have a "
                    "valid renderer."));
        return;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!RiveFile.IsValid() || !RiveFile->GetNativeFile())
    {
        return;
    }

    // If we have a RiveFile reference, it means we've started up before. Make
    // sure if the RiveFile is initializing, we deinitialize
    if (!RiveFileDeinitializationHandle.IsValid())
    {
        RiveFileDeinitializationHandle =
            RiveFile->OnStartInitializingDelegate.AddUObject(
                this,
                &URiveArtboard::Deinitialize);
    }

    if (!RiveFileReinitializationHandle.IsValid())
    {
        RiveFileReinitializationHandle =
            RiveFile->OnInitializedDelegate.AddUObject(
                this,
                &URiveArtboard::Reinitialize);
    }

    rive::Artboard* NativeArtboard = nullptr;

    if (InName.IsEmpty())
    {
        NativeArtboard = RiveFile->GetNativeFile()->artboard();
    }
    else
    {
        NativeArtboard =
            RiveFile->GetNativeFile()->artboard(RiveUtils::ToUTF8(*InName));
        if (!NativeArtboard)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("Could not initialize the artboard by the name '%s'. "
                        "Initializing with "
                        "default artboard instead"),
                   *InName);
            NativeArtboard = RiveFile->GetNativeFile()->artboard();
        }
    }
    Initialize_Internal(NativeArtboard);
}

void URiveArtboard::Reinitialize(bool InSuccess)
{
    if (this == nullptr || !InSuccess || !RiveFile.IsValid())
        return;

    if (ArtboardName.IsEmpty())
    {
        Initialize(RiveFile.Get(),
                   RiveRenderTarget,
                   ArtboardIndex,
                   StateMachineName);
    }
    else
    {
        Initialize(RiveFile.Get(),
                   RiveRenderTarget,
                   ArtboardName,
                   StateMachineName);
    }
}

void URiveArtboard::SetStateMachineName(const FString& NewStateMachineName)
{
    if (StateMachineName != NewStateMachineName)
    {
        StateMachineName = NewStateMachineName;

        StateMachinePtr = MakeUnique<FRiveStateMachine>(NativeArtboardPtr.get(),
                                                        StateMachineName);

        if (CurrentViewModelInstance.IsValid())
            StateMachinePtr->SetViewModelInstance(
                CurrentViewModelInstance.Get());
    }
}

void URiveArtboard::SetAudioEngine(URiveAudioEngine* AudioEngine)
{
    if (AudioEngine == nullptr)
    {
        rive::rcp<rive::AudioEngine> NativeEngine =
            NativeArtboardPtr->audioEngine();

        if (NativeEngine != nullptr)
        {
            NativeEngine->unref();
        }
        NativeArtboardPtr->audioEngine(nullptr);
        return;
    }
    NativeArtboardPtr->audioEngine(AudioEngine->GetNativeAudioEngine());
}

void URiveArtboard::Tick_Render(float InDeltaSeconds)
{
    SCOPED_NAMED_EVENT_TEXT("URiveArtboard::Tick_Render", FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RiveArtboard::Tick_Render"),
                                STAT_RIVEARTBOARD_TICKRENDER,
                                STATGROUP_Rive);
    if (OnArtboardTick_Render.IsBound())
    {
        OnArtboardTick_Render.Execute(InDeltaSeconds, this);
    }
    else
    {
        Draw();
    }
}

void URiveArtboard::Tick_StateMachine(float InDeltaSeconds)
{
    SCOPED_NAMED_EVENT_TEXT("URiveArtboard::Tick_StateMachine", FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("RiveArtboard::Tick_StateMachine"),
                                STAT_RIVEARTBOARD_TICKSTATEMACHINE,
                                STATGROUP_Rive);
    if (OnArtboardTick_StateMachine.IsBound())
    {
        OnArtboardTick_StateMachine.Execute(InDeltaSeconds, this);
    }
    else
    {
        AdvanceStateMachine(InDeltaSeconds);
    }
}

void URiveArtboard::Deinitialize()
{
    if (this == nullptr)
        return;
    bIsInitialized = false;

    StateMachinePtr.Reset();
    if (NativeArtboardPtr != nullptr)
    {
        NativeArtboardPtr.release();
    }
    NativeArtboardPtr.reset();
    OnArtboardTick_Render.Clear();
    OnArtboardTick_StateMachine.Clear();
}

void URiveArtboard::Tick(float InDeltaSeconds)
{
    if (!RiveRenderTarget || !bIsInitialized)
    {
        return;
    }

    Tick_StateMachine(InDeltaSeconds);
    Tick_Render(InDeltaSeconds);
}

rive::ArtboardInstance* URiveArtboard::GetNativeArtboard() const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to retrieve the NativeArtboard as we do not have"
                    " a valid renderer."));
        return nullptr;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeArtboardPtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Could not retrieve artboard as we have detected an empty "
                    "rive artboard."));

        return nullptr;
    }

    return NativeArtboardPtr.get();
}

rive::AABB URiveArtboard::GetBounds() const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to GetBounds for the Artboard as we do not have"
                    " a valid renderer."));
        return {};
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeArtboardPtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Could not retrieve artboard bounds as we have detected an "
                    "empty rive artboard."));

        return {0, 0, 0, 0};
    }

    return NativeArtboardPtr->bounds();
}

FVector2f URiveArtboard::GetOriginalSize() const
{
    if (!NativeArtboardPtr)
        return FVector2f::ZeroVector;

    return FVector2f(NativeArtboardPtr->originalWidth(),
                     NativeArtboardPtr->originalHeight());
}

FVector2f URiveArtboard::GetSize() const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to GetSize for the Artboard as we do not have a "
                    "valid renderer."));
        return {};
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeArtboardPtr)
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("Could not retrieve artboard size as we have detected an "
                    "empty rive artboard."));

        return FVector2f::ZeroVector;
    }

    return {NativeArtboardPtr->width(), NativeArtboardPtr->height()};
}

void URiveArtboard::SetSize(FVector2f InVector)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to SetSize for the Artboard as we do not have a "
                    "valid renderer."));
        return;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!NativeArtboardPtr)
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("Could not set artboard size as we have detected an "
                    "empty rive artboard."));

        return;
    }

    NativeArtboardPtr->width(InVector.X);
    NativeArtboardPtr->height(InVector.Y);

    return;
}

FRiveStateMachine* URiveArtboard::GetStateMachine() const
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to GetStateMachine for the Artboard as we do not "
                    "have a valid renderer."));
        return nullptr;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (!StateMachinePtr)
    {
        // Not all artboards have state machines, so let's not error it out
        return nullptr;
    }

    return StateMachinePtr.Get();
}

void URiveArtboard::PopulateReportedEvents()
{
#if WITH_RIVE
    TickRiveReportedEvents.Empty();

    if (const FRiveStateMachine* StateMachine = GetStateMachine())
    {
        const int32 NumReportedEvents = StateMachine->GetReportedEventsCount();
        TickRiveReportedEvents.Reserve(NumReportedEvents);

        for (int32 EventIndex = 0; EventIndex < NumReportedEvents; EventIndex++)
        {
            const rive::EventReport ReportedEvent =
                StateMachine->GetReportedEvent(EventIndex);
            if (ReportedEvent.event() != nullptr)
            {
                FRiveEvent RiveEvent;
                RiveEvent.Initialize(ReportedEvent);
                if (const FRiveNamedEventsDelegate* NamedEventDelegate =
                        NamedRiveEventsDelegates.Find(RiveEvent.Name))
                {
                    NamedEventDelegate->Broadcast(this, RiveEvent);
                }
                TickRiveReportedEvents.Add(MoveTemp(RiveEvent));
            }
        }

        if (!TickRiveReportedEvents.IsEmpty())
        {
            RiveEventDelegate.Broadcast(this, TickRiveReportedEvents);
        }
    }
    else
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("Failed to populate reported event(s) as we could not "
                    "retrieve native state "
                    "machine."));
    }

#endif // WITH_RIVE
}

void URiveArtboard::Initialize_Internal(const rive::Artboard* InNativeArtboard)
{
    NativeArtboardPtr = InNativeArtboard->instance();
    if (!NativeArtboardPtr)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("URiveArtboard::Initialize_Internal "
                    "InNativeArtboard->instance() is null !"));
        return;
    }

    ArtboardName = FString{NativeArtboardPtr->name().c_str()};
    NativeArtboardPtr->advance(0);

    // UI Helpers
    StateMachineNames.Empty();
    for (int i = 0; i < NativeArtboardPtr->stateMachineCount(); ++i)
    {
        const rive::StateMachine* NativeStateMachine =
            NativeArtboardPtr->stateMachine(i);
        StateMachineNames.Add(NativeStateMachine->name().c_str());
    }

    StateMachinePtr = MakeUnique<FRiveStateMachine>(NativeArtboardPtr.get(),
                                                    StateMachineName);

    // Update our active StateMachineNAme with our actual state machine name
    StateMachineName = StateMachinePtr->GetStateMachineName();

    EventNames.Empty();
    const std::vector<rive::Event*> Events =
        NativeArtboardPtr->find<rive::Event>();
    for (const rive::Event* Event : Events)
    {
        EventNames.Add(Event->name().c_str());
    }

    bIsInitialized = true;
}

void URiveArtboard::SetViewModelInstance(
    URiveViewModelInstance* RiveViewModelInstance)
{
    // Store off the VM instance because we need to set
    // it on dynamically created state machines.
    CurrentViewModelInstance = RiveViewModelInstance;

    if (!NativeArtboardPtr)
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("SetViewModelInstance failed: "
                    "NativeArtboardPtr is null."));
        return;
    }

    if (!RiveViewModelInstance || !RiveViewModelInstance->GetNativePtr())
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("SetViewModelInstance failed: "
                    "RuntimeInstance is invalid or null."));
        return;
    }

    // Access the native Rive runtime instance
    rive::ViewModelInstanceRuntime* NativeInstance =
        RiveViewModelInstance->GetNativePtr();

    // Set the data context on the artboard
    NativeArtboardPtr->bindViewModelInstance(NativeInstance->instance());

    // Set the data context on the state machine if it exists.
    FRiveStateMachine* StateMachine = GetStateMachine();
    if (StateMachine)
        StateMachine->SetViewModelInstance(RiveViewModelInstance);
}

#endif // WITH_RIVE
