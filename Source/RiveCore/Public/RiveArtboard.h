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
	DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FVector2f, FRiveCoordinatesDelegate, URiveArtboard*, Artboard, const FVector2f&, TexturePosition);
	DECLARE_DYNAMIC_DELEGATE_TwoParams(FRiveTickDelegate, float, DeltaTime, URiveArtboard*, Artboard);
	
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive, meta=(GetOptions="GetStateMachineNamesForDropdown"))
	FString StateMachineName;
	
	UPROPERTY(BlueprintReadWrite, Category = Rive)
	FRiveTickDelegate OnArtboardTick_Render;

	UPROPERTY(BlueprintReadWrite, Category = Rive)
	FRiveTickDelegate OnArtboardTick_StateMachine;

	UPROPERTY(BlueprintReadWrite, Category = Rive)
	FRiveCoordinatesDelegate OnGetLocalCoordinate;
	
	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetSize() const;
	
	UFUNCTION(BlueprintCallable, Category = Rive)
	void AdvanceStateMachine(float InDeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = Rive)
	void Transform(const FVector2f& One, const FVector2f& Two, const FVector2f& T);
	
	UFUNCTION(BlueprintCallable, Category = Rive)
	void Translate(const FVector2f& InVector);
	
	UFUNCTION(BlueprintCallable, Category = Rive)
	void Align(const FBox2f InBox, ERiveFitType InFitType, ERiveAlignment InAlignment);
	
	void Align(ERiveFitType InFitType, ERiveAlignment InAlignment);

	/** Returns the transformation Matrix from the start of the Render Queue up to now */
	UFUNCTION(BlueprintCallable, Category = Rive)
	FMatrix GetTransformMatrix() const;

	/** Returns the transformation Matrix from the start of the Render Queue up to now */
	UFUNCTION(BlueprintCallable, Category = Rive)
	FMatrix GetLastDrawTransformMatrix() const { return LastDrawTransform; }

	UFUNCTION(BlueprintCallable, Category = Rive)
	void Draw();
	
	UFUNCTION(BlueprintCallable, Category = Rive)
	void FireTrigger(const FString& InPropertyName) const;
	UFUNCTION(BlueprintCallable, Category = Rive)
	bool GetBoolValue(const FString& InPropertyName) const;
	UFUNCTION(BlueprintCallable, Category = Rive)
	float GetNumberValue(const FString& InPropertyName) const;
	UFUNCTION(BlueprintCallable, Category = Rive)
	FString GetTextValue(const FString& InPropertyName) const;
	
	UFUNCTION(BlueprintCallable, Category = Rive)
	void SetBoolValue(const FString& InPropertyName, bool bNewValue);
	UFUNCTION(BlueprintCallable, Category = Rive)
	void SetNumberValue(const FString& InPropertyName, float NewValue);
	UFUNCTION(BlueprintCallable, Category = Rive)
	void SetTextValue(const FString& InPropertyName, const FString& NewValue);
	
	UFUNCTION(BlueprintCallable, Category = Rive)
	bool BindNamedRiveEvent(const FString& EventName, const FRiveNamedEventDelegate& Event);
	UFUNCTION(BlueprintCallable, Category = Rive)
	bool UnbindNamedRiveEvent(const FString& EventName, const FRiveNamedEventDelegate& Event);
	UFUNCTION(BlueprintCallable, Category = Rive)
	bool TriggerNamedRiveEvent(const FString& EventName, float ReportedDelaySeconds);


	// Used to convert from a given point (InPosition) on a texture local position
	// to the position for this artboard, taking into account alignment, fit, and an offset (if custom translation has been used)
	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetLocalCoordinate(const FVector2f& InPosition, const FIntPoint& InTextureSize, ERiveAlignment InAlignment, ERiveFitType InFit) const;
	
	/**
	 * Returns the coordinates in the current Artboard space
	 * @param InExtents Extents of the RenderTarget, will be mapped to the RenderTarget size
	 */
	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetLocalCoordinatesFromExtents(const FVector2f& InPosition, const FBox2f& InExtents, const FIntPoint& TextureSize, ERiveAlignment Alignment, ERiveFitType FitType) const;
	
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

	/** The Matrix at the time of the last call to Draw for this Artboard **/
	FMatrix LastDrawTransform = FMatrix::Identity;

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
