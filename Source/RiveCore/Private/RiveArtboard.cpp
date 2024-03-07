// Copyright Rive, Inc. All rights reserved.

#include "RiveArtboard.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "RiveEvent.h"
#include "Logs/RiveCoreLog.h"
#include "URStateMachine.h"

#if WITH_RIVE
#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
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
	bIsInitialized = false;

	DefaultStateMachinePtr.Reset();
	if (NativeArtboardPtr != nullptr)
	{
		NativeArtboardPtr.release();
	}
	NativeArtboardPtr.reset();
	OnArtboardTick_Render.Clear();
	OnArtboardTick_StateMachine.Clear();
	UObject::BeginDestroy();
}

void URiveArtboard::AdvanceStateMachine(float InDeltaSeconds)
{
	UE::Rive::Core::FURStateMachine* StateMachine = GetStateMachine();
	if (StateMachine && StateMachine->IsValid() && ensure(RiveRenderTarget))
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
}

void URiveArtboard::Transform(const FVector2f& One, const FVector2f& Two, const FVector2f& T)
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

void URiveArtboard::Align(const FBox2f InBox, ERiveFitType InFitType, ERiveAlignment InAlignment)
{
	if (!RiveRenderTarget)
	{
		return;
	}
	RiveRenderTarget->Align(InBox, InFitType, FRiveAlignment::GetAlignment(InAlignment), GetNativeArtboard());
}

void URiveArtboard::Align(ERiveFitType InFitType, ERiveAlignment InAlignment)
{
	if (!RiveRenderTarget)
	{
		return;
	}
	RiveRenderTarget->Align(InFitType, FRiveAlignment::GetAlignment(InAlignment), GetNativeArtboard());
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
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (ensure(RiveRenderer))
	{
		FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
		if (const UE::Rive::Core::FURStateMachine* StateMachine = GetStateMachine())
		{
			StateMachine->FireTrigger(InPropertyName);
		}
	}
}

bool URiveArtboard::GetBoolValue(const FString& InPropertyName) const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (ensure(RiveRenderer))
	{
		FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
		if (const UE::Rive::Core::FURStateMachine* StateMachine = GetStateMachine())
		{
			return StateMachine->GetBoolValue(InPropertyName);
		}
	}
	return false;
}

float URiveArtboard::GetNumberValue(const FString& InPropertyName) const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (ensure(RiveRenderer))
	{
		FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
		if (const UE::Rive::Core::FURStateMachine* StateMachine = GetStateMachine())
		{
			return StateMachine->GetNumberValue(InPropertyName);
		}
	}
	return 0.f;
}

FString URiveArtboard::GetTextValue(const FString& InPropertyName) const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (ensure(RiveRenderer))
	{
		FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
		if (const UE::Rive::Core::FURStateMachine* StateMachine = GetStateMachine())
		{
			if (const rive::TextValueRunBase* TextValueRun = NativeArtboardPtr->find<rive::TextValueRunBase>(TCHAR_TO_UTF8(*InPropertyName)))
			{
				return FString{TextValueRun->text().c_str()};
			}
		}
	}
	return {};
}

void URiveArtboard::SetBoolValue(const FString& InPropertyName, bool bNewValue)
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (ensure(RiveRenderer))
	{
		FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
		if (UE::Rive::Core::FURStateMachine* StateMachine = GetStateMachine())
		{
			StateMachine->SetBoolValue(InPropertyName, bNewValue);
		}
	}
}

void URiveArtboard::SetNumberValue(const FString& InPropertyName, float NewValue)
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (ensure(RiveRenderer))
	{
		FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
		if (UE::Rive::Core::FURStateMachine* StateMachine = GetStateMachine())
		{
			StateMachine->SetNumberValue(InPropertyName, NewValue);
		}
	}
}

void URiveArtboard::SetTextValue(const FString& InPropertyName, const FString& NewValue)
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (ensure(RiveRenderer))
	{
		FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
		if (const UE::Rive::Core::FURStateMachine* StateMachine = GetStateMachine())
		{
			if (rive::TextValueRunBase* TextValueRun = NativeArtboardPtr->find<rive::TextValueRunBase>(TCHAR_TO_UTF8(*InPropertyName)))
			{
				TextValueRun->text(TCHAR_TO_UTF8(*NewValue));
			}
		}
	}
}

bool URiveArtboard::BindNamedRiveEvent(const FString& EventName, const FRiveNamedEventDelegate& Event)
{
	if (EventNames.Contains(EventName))
	{
		NamedRiveEventsDelegates.FindOrAdd(EventName, {}).AddUnique(Event);
		return true;
	}
	UE_LOG(LogRiveCore, Error, TEXT("Unable to bind event '%s' to Artboard '%s' as the event does not exist"), *EventName, *GetArtboardName())
	return false;
}

bool URiveArtboard::UnbindNamedRiveEvent(const FString& EventName, const FRiveNamedEventDelegate& Event)
{
	if (EventNames.Contains(EventName))
	{
		if (FRiveNamedEventsDelegate* NamedRiveDelegate = NamedRiveEventsDelegates.Find(EventName))
		{
			NamedRiveDelegate->Remove(Event);
			if (!NamedRiveDelegate->IsBound())
			{
				NamedRiveEventsDelegates.Remove(EventName);
			}
		}
		return true;
	}
	UE_LOG(LogRiveCore, Error, TEXT("Unable to bind event '%s' to Artboard '%s' as the event does not exist"), *EventName, *GetArtboardName())
	return false;
}

bool URiveArtboard::TriggerNamedRiveEvent(const FString& EventName, float ReportedDelaySeconds)
{
	if (NativeArtboardPtr && GetStateMachine())
	{
		if (rive::Component* Component = NativeArtboardPtr->find(TCHAR_TO_UTF8(*EventName)))
		{
			if(Component->is<rive::Event>())
			{
				rive::Event* Event = Component->as<rive::Event>();
				const rive::CallbackData CallbackData(GetStateMachine()->GetNativeStateMachinePtr().get(), ReportedDelaySeconds);
				Event->trigger(CallbackData);
				UE_LOG(LogRiveCore, Warning, TEXT("TRIGGERED event '%s' for Artboard '%s'"), *EventName, *GetArtboardName())
				return true;
			}
		}
		UE_LOG(LogRiveCore, Error, TEXT("Unable to trigger event '%s' for Artboard '%s' as the Artboard is not ready or it doesn't have a state machine"), *EventName, *GetArtboardName())
	}
	else
	{
		UE_LOG(LogRiveCore, Error, TEXT("Unable to trigger event '%s' for Artboard '%s' as the event does not exist"), *EventName, *GetArtboardName())
	}
	
	return false;
}

FVector2f URiveArtboard::GetLocalCoordinate(const FVector2f& InPosition, const FIntPoint& InTextureSize, ERiveAlignment InAlignment, ERiveFitType InFit) const
{
	FVector2f Alignment = FRiveAlignment::GetAlignment(InAlignment);
	rive::Mat2D Transform = rive::computeAlignment(
					static_cast<rive::Fit>(InFit),
					rive::Alignment(Alignment.X, Alignment.Y),
					rive::AABB(0.f, 0.f, InTextureSize.X, InTextureSize.Y),
					NativeArtboardPtr->bounds());

	rive::Vec2D Vector = Transform.invertOrIdentity() * rive::Vec2D(InPosition.X, InPosition.Y);
	return {Vector.x, Vector.y};
}

FVector2f URiveArtboard::GetLocalCoordinatesFromExtents(const FVector2f& InPosition, const FBox2f& InExtents, const FIntPoint& TextureSize, ERiveAlignment Alignment, ERiveFitType FitType) const
{
	const FVector2f RelativePosition = InPosition - InExtents.Min;
	const FVector2f Ratio { TextureSize.X / InExtents.GetSize().X, TextureSize.Y / InExtents.GetSize().Y}; // Ratio should be the same for X and Y
	const FVector2f TextureRelativePosition = RelativePosition * Ratio;
	
	return GetLocalCoordinate(TextureRelativePosition, TextureSize, Alignment, FitType);
}

void URiveArtboard::Initialize(rive::File* InNativeFilePtr, const UE::Rive::Renderer::IRiveRenderTargetPtr& InRiveRenderTarget)
{
	Initialize(InNativeFilePtr, InRiveRenderTarget, 0, "");
}

void URiveArtboard::Initialize(rive::File* InNativeFilePtr, UE::Rive::Renderer::IRiveRenderTargetPtr InRiveRenderTarget,
                               int32 InIndex, const FString& InStateMachineName)
{
	RiveRenderTarget = InRiveRenderTarget;
	StateMachineName = InStateMachineName;
	
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (!RiveRenderer)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Failed to Initialize the Artboard as we do not have a valid renderer."));
		return;
	}
	
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	if (!InNativeFilePtr)
	{
		return;
	}

	int32 Index = InIndex;
	if (Index >= InNativeFilePtr->artboardCount())
	{
		Index = InNativeFilePtr->artboardCount() - 1;
		UE_LOG(LogRiveCore, Warning,
		       TEXT(
			       "Artboard index specified is out of bounds, using the last available artboard index instead, which is %d"
		       ), Index);
	}

	if (const rive::Artboard* NativeArtboard = InNativeFilePtr->artboard(Index))
	{
		Initialize_Internal(NativeArtboard);
	}
}

void URiveArtboard::Initialize(rive::File* InNativeFilePtr, UE::Rive::Renderer::IRiveRenderTargetPtr InRiveRenderTarget,
                               const FString& InName, const FString& InStateMachineName)
{
	RiveRenderTarget = InRiveRenderTarget;
	StateMachineName = InStateMachineName;
	
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (!RiveRenderer)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Failed to Initialize the Artboard as we do not have a valid renderer."));
		return;
	}
	
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	if (!InNativeFilePtr)
	{
		return;
	}

	rive::Artboard* NativeArtboard = nullptr;
	
	if (InName.IsEmpty())
	{
		NativeArtboard = InNativeFilePtr->artboard();
	} else
	{
		NativeArtboard = InNativeFilePtr->artboard(TCHAR_TO_UTF8(*InName));
		if (!NativeArtboard)
		{
			UE_LOG(LogRiveCore, Error,
				TEXT("Could not initialize the artboard by the name '%s'. Initializing with default artboard instead"),
				*InName);
			NativeArtboard = InNativeFilePtr->artboard();
		}
	}
	Initialize_Internal(NativeArtboard);
}

void URiveArtboard::Tick_Render(float InDeltaSeconds)
{
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
	if (OnArtboardTick_StateMachine.IsBound())
	{
		OnArtboardTick_StateMachine.Execute(InDeltaSeconds, this);
	}
	else
	{
		AdvanceStateMachine(InDeltaSeconds);
	}
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

rive::Artboard* URiveArtboard::GetNativeArtboard() const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (!RiveRenderer)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Failed to retrieve the NativeArtboard as we do not have a valid renderer."));
		return nullptr;
	}
	
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!NativeArtboardPtr)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Could not retrieve artboard as we have detected an empty rive artboard."));

		return nullptr;
	}

	return NativeArtboardPtr.get();
}

rive::AABB URiveArtboard::GetBounds() const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (!RiveRenderer)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Failed to GetBounds for the Artboard as we do not have a valid renderer."));
		return {};
	}
	
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!NativeArtboardPtr)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Could not retrieve artboard bounds as we have detected an empty rive artboard."));

		return {0, 0, 0, 0};
	}

	return NativeArtboardPtr->bounds();
}

FVector2f URiveArtboard::GetSize() const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (!RiveRenderer)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Failed to GetSize for the Artboard as we do not have a valid renderer."));
		return {};
	}
	
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!NativeArtboardPtr)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Could not retrieve artboard size as we have detected an empty rive artboard."));

		return FVector2f::ZeroVector;
	}

	return {NativeArtboardPtr->width(), NativeArtboardPtr->height()};
}

UE::Rive::Core::FURStateMachine* URiveArtboard::GetStateMachine() const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	if (!RiveRenderer)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Failed to GetStateMachine for the Artboard as we do not have a valid renderer."));
		return nullptr;
	}
	
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!DefaultStateMachinePtr)
	{
		// Not all artboards have state machines, so let's not error it out
		return nullptr;
	}

	return DefaultStateMachinePtr.Get();
}

void URiveArtboard::PopulateReportedEvents()
{
#if WITH_RIVE
	TickRiveReportedEvents.Empty();
	
	if (const UE::Rive::Core::FURStateMachine* StateMachine = GetStateMachine())
	{
		const int32 NumReportedEvents = StateMachine->GetReportedEventsCount();
		TickRiveReportedEvents.Reserve(NumReportedEvents);

		for (int32 EventIndex = 0; EventIndex < NumReportedEvents; EventIndex++)
		{
			const rive::EventReport ReportedEvent = StateMachine->GetReportedEvent(EventIndex);
			if (ReportedEvent.event() != nullptr)
			{
				FRiveEvent RiveEvent;
				RiveEvent.Initialize(ReportedEvent);
				if (const FRiveNamedEventsDelegate* NamedEventDelegate = NamedRiveEventsDelegates.Find(RiveEvent.Name))
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
		UE_LOG(LogRiveCore, Error, TEXT("Failed to populate reported event(s) as we could not retrieve native state machine."));
	}

#endif // WITH_RIVE
}

void URiveArtboard::Initialize_Internal(const rive::Artboard* InNativeArtboard)
{
	NativeArtboardPtr = InNativeArtboard->instance();
	ArtboardName = FString{NativeArtboardPtr->name().c_str()};
	NativeArtboardPtr->advance(0);

	DefaultStateMachinePtr = MakeUnique<UE::Rive::Core::FURStateMachine>(
		NativeArtboardPtr.get(), StateMachineName);

	// UI Helpers
	StateMachineNames.Empty();
	for (int i = 0; i < NativeArtboardPtr->stateMachineCount(); ++i)
	{
		const rive::StateMachine* NativeStateMachine = NativeArtboardPtr->stateMachine(i);
		StateMachineNames.Add(NativeStateMachine->name().c_str());
	}
	
	EventNames.Empty();
	const std::vector<rive::Event*> Events = NativeArtboardPtr->find<rive::Event>();
	for (const rive::Event* Event : Events)
	{
		EventNames.Add(Event->name().c_str());
	}

	BoolInputNames.Empty();
	NumberInputNames.Empty();
	TriggerInputNames.Empty();
	if (DefaultStateMachinePtr && DefaultStateMachinePtr.IsValid())
	{
		for (uint32 i = 0; i < DefaultStateMachinePtr->GetInputCount(); ++i)
		{
			const rive::SMIInput* Input = DefaultStateMachinePtr->GetInput(i);
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
				UE_LOG(LogRiveCore, Warning, TEXT("Found input of unknown type '%d' when getting inputs from StateMachine '%s' from Artboard '%hs'"),
					Input->inputCoreType(), *DefaultStateMachinePtr->GetStateMachineName(), InNativeArtboard->name().c_str())
			}
		}
	}
	
	bIsInitialized = true;
}

#endif // WITH_RIVE
