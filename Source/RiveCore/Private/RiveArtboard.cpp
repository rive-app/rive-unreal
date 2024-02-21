// Copyright Rive, Inc. All rights reserved.

#include "RiveArtboard.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveCoreLog.h"
#include "URStateMachine.h"

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
	
	UObject::BeginDestroy();
}

void URiveArtboard::Initialize(rive::File* InNativeFilePtr)
{
	Initialize(InNativeFilePtr, 0);
}

void URiveArtboard::Initialize(rive::File* InNativeFilePtr, int32 InIndex, const FString& InStateMachineName)
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!InNativeFilePtr)
	{
		return;
	}
	
	StateMachineName = InStateMachineName;

	if (InIndex >= InNativeFilePtr->artboardCount())
	{
		InIndex = InNativeFilePtr->artboardCount() - 1;
		UE_LOG(LogRiveCore, Warning,
		       TEXT(
			       "Artboard index specified is out of bounds, using the last available artboard index instead, which is %d"
		       ), InIndex);
	}

	if (rive::Artboard* NativeArtboard = InNativeFilePtr->artboard(InIndex))
	{
		NativeArtboardPtr = NativeArtboard->instance();

		NativeArtboardPtr->advance(0);

		DefaultStateMachinePtr = MakeUnique<UE::Rive::Core::FURStateMachine>(NativeArtboardPtr.get(), StateMachineName);
	}

	bIsInitialized = true;
}

void URiveArtboard::Initialize(rive::File* InNativeFilePtr, const FString& InName, const FString& InStateMachineName)
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!InNativeFilePtr)
	{
		return;
	}

	StateMachineName = InStateMachineName;
	rive::Artboard* NativeArtboard = InNativeFilePtr->artboard(TCHAR_TO_UTF8(*InName));

	if (!NativeArtboard)
	{
		UE_LOG(LogRiveCore, Error,
		       TEXT("Could not initialize the artboard by the name '%s'. Initializing with default artboard instead"),
		       *InName);
		NativeArtboard = InNativeFilePtr->artboard();
	}

	NativeArtboardPtr = NativeArtboard->instance();

	NativeArtboardPtr->advance(0);

	DefaultStateMachinePtr = MakeUnique<UE::Rive::Core::FURStateMachine>(NativeArtboardPtr.get(), StateMachineName);

	bIsInitialized = true;
}

rive::Artboard* URiveArtboard::GetNativeArtboard() const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!NativeArtboardPtr)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Could not retrieve artboard as we have detected an empty rive atrboard."));

		return nullptr;
	}

	return NativeArtboardPtr.get();
}

rive::AABB URiveArtboard::GetBounds() const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!NativeArtboardPtr)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Could not retrieve artboard bounds as we have detected an empty rive atrboard."));

		return {0, 0, 0, 0};
	}

	return NativeArtboardPtr->bounds();
}

FVector2f URiveArtboard::GetSize() const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!NativeArtboardPtr)
	{
		UE_LOG(LogRiveCore, Error, TEXT("Could not retrieve artboard size as we have detected an empty rive atrboard."));

		return FVector2f::ZeroVector;
	}

	return {NativeArtboardPtr->width(), NativeArtboardPtr->height()};
}

UE::Rive::Core::FURStateMachine* URiveArtboard::GetStateMachine() const
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (!DefaultStateMachinePtr)
	{
		// Not all artboards have state machines, so let's not error it out
		return nullptr;
	}

	return DefaultStateMachinePtr.Get();
}

#endif // WITH_RIVE
