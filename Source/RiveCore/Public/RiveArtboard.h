// Copyright Rive, Inc. All rights reserved.
#pragma once
#include "IRiveRenderTarget.h"
#include "MatrixTypes.h"
#include "RiveEvent.h"
#include "RiveTypes.h"
#include "URStateMachine.h"

#if WITH_RIVE
class URiveFile;

#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/file.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

#include "RiveArtboard.generated.h"



UCLASS(BlueprintType)
class RIVECORE_API URiveArtboard : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRiveEventDelegate, URiveArtboard*, Artboard, TArray<FRiveEvent>, ReportedEvents);
	DECLARE_DYNAMIC_DELEGATE_TwoParams(FRiveNamedEventDelegate, URiveArtboard*, Artboard, FRiveEvent, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRiveNamedEventsDelegate, URiveArtboard*, Artboard, FRiveEvent, Event);
	
	DECLARE_DYNAMIC_DELEGATE_TwoParams(FRiveTickDelegate, float, DeltaTime, URiveArtboard*, Artboard);
	
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive, meta=(GetOptions="GetStateMachineNamesForDropdown"))
	FString StateMachineName;
	
	UPROPERTY(BlueprintReadWrite)
	FRiveTickDelegate OnArtboardTick_Render;

	UPROPERTY(BlueprintReadWrite)
	FRiveTickDelegate OnArtboardTick_StateMachine;

	UFUNCTION(BlueprintCallable)
	FVector2f GetSize() const;
	
	UFUNCTION(BlueprintCallable)
	void AdvanceStateMachine(float InDeltaSeconds);

	UFUNCTION(BlueprintCallable)
	void Transform(const FVector2f& One, const FVector2f& Two, const FVector2f& T);
	
	UFUNCTION(BlueprintCallable)
	void Translate(const FVector2f& InVector);
	
	UFUNCTION(BlueprintCallable)
	void Align(ERiveFitType InFitType, ERiveAlignment InAlignment);

	UFUNCTION(BlueprintCallable)
	void Draw();

	UFUNCTION(BlueprintCallable)
	bool BindNamedRiveEvent(const FString& EventName, const FRiveNamedEventDelegate& Event);
	UFUNCTION(BlueprintCallable)
	bool UnbindNamedRiveEvent(const FString& EventName, const FRiveNamedEventDelegate& Event);
	UFUNCTION(BlueprintCallable)
	bool TriggerNamedRiveEvent(const FString& EventName, float ReportedDelaySeconds);
	
#if WITH_RIVE
	
	void Initialize(rive::File* InNativeFilePtr, const UE::Rive::Renderer::IRiveRenderTargetPtr& InRiveRenderTarget);
	void Initialize(rive::File* InNativeFilePtr, UE::Rive::Renderer::IRiveRenderTargetPtr InRiveRenderTarget, int32 InIndex, const FString& InStateMachineName);
	void Initialize(rive::File* InNativeFilePtr, UE::Rive::Renderer::IRiveRenderTargetPtr InRiveRenderTarget, const FString& InName, const FString& InStateMachineName);
	void SetRenderTarget(const UE::Rive::Renderer::IRiveRenderTargetPtr& InRiveRenderTarget) { RiveRenderTarget = InRiveRenderTarget; }
	
	bool IsInitialized() const { return bIsInitialized; }

	void Tick(float InDeltaSeconds);
	/**
	 * Implementation(s)
	 */

public:
	
	rive::Artboard* GetNativeArtboard() const;

	rive::AABB GetBounds() const;

	UE::Rive::Core::FURStateMachine* GetStateMachine() const;

	void BeginInput()
	{
		bIsReceivingInput = true;
	}
	
	void EndInput()
	{
		bIsReceivingInput = false;
	}
	/**
	 * Attribute(s)
	 */

private:
	void PopulateReportedEvents();
	
	void Initialize_Internal(const rive::Artboard* InNativeArtboard);
	void Tick_Render(float InDeltaSeconds);
	void Tick_StateMachine(float InDeltaSeconds);
	
	UE::Rive::Renderer::IRiveRenderTargetPtr RiveRenderTarget;
	mutable bool bIsInitialized = false;

	std::unique_ptr<rive::ArtboardInstance> NativeArtboardPtr = nullptr;
	UE::Rive::Core::FURStateMachinePtr DefaultStateMachinePtr = nullptr;
#endif // WITH_RIVE
public:
	const FString& GetArtboardName() const { return ArtboardName; }
	const TArray<FString>& GetEventNames() const { return EventNames; }
private:
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess))
	FString ArtboardName;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess))
	TArray<FString> BoolInputNames;
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess))
	TArray<FString> NumberInputNames;
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess))
	TArray<FString> TriggerInputNames;
	
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess))
	TArray<FString> EventNames;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess)) //todo: unexpose to BP and UI
	TMap<FString, FRiveNamedEventsDelegate> NamedRiveEventsDelegates;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess))
	TArray<FString> StateMachineNames;

public:
	UFUNCTION()
	TArray<FString> GetStateMachineNamesForDropdown() const
	{
		TArray<FString> Names {FString{}};
		Names.Append(StateMachineNames);
		return Names;
	}
protected:
	UPROPERTY(BlueprintAssignable)
	FRiveEventDelegate RiveEventDelegate;

	UPROPERTY(BlueprintReadWrite, Category = Rive)
	TArray<FRiveEvent> TickRiveReportedEvents;
	
	bool bIsReceivingInput = false;
};
