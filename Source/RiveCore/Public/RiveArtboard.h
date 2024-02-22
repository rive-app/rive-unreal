// Copyright Rive, Inc. All rights reserved.
#pragma once
#include "IRiveRenderTarget.h"
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



UCLASS()
class RIVECORE_API URiveArtboard : public UObject
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive)
	FString StateMachineName;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRiveArtboardTick, float, DeltaTime);
	UPROPERTY(BlueprintAssignable)
	FOnRiveArtboardTick OnArtboardTick_Render;

	UPROPERTY(BlueprintAssignable)
	FOnRiveArtboardTick OnArtboardTick_StateMachine;

	UPROPERTY(EditAnywhere, Category = Rive)
	ERiveFitType RiveFitType = ERiveFitType::Contain;

	/* This property is not editable via Editor in Unity, so we'll hide it also */
	UPROPERTY()
	ERiveAlignment RiveAlignment = ERiveAlignment::Center;

	UFUNCTION(BlueprintCallable)
	void AdvanceStateMachine(float InDeltaSeconds);

	UFUNCTION(BlueprintCallable)
	void Align(ERiveFitType InFitType, ERiveAlignment InAlignment);

	UFUNCTION(BlueprintCallable)
	void Draw();
	
	
#if WITH_RIVE

	void Initialize(rive::File* InNativeFilePtr, UE::Rive::Renderer::IRiveRenderTargetPtr InRiveRenderTarget);
	void Initialize(rive::File* InNativeFilePtr, UE::Rive::Renderer::IRiveRenderTargetPtr InRiveRenderTarget, int32 InIndex, const FString& InStateMachineName = TEXT_EMPTY, ERiveFitType InFitType = ERiveFitType::Cover, ERiveAlignment InAlignment = ERiveAlignment::Center);
	void Initialize(rive::File* InNativeFilePtr, UE::Rive::Renderer::IRiveRenderTargetPtr InRiveRenderTarget, const FString& InName, const FString& InStateMachineName = TEXT_EMPTY, ERiveFitType InFitType = ERiveFitType::Cover, ERiveAlignment InAlignment = ERiveAlignment::Center);
	void SetRenderTarget(const UE::Rive::Renderer::IRiveRenderTargetPtr& InRiveRenderTarget) { RiveRenderTarget = InRiveRenderTarget; }
	
	bool IsInitialized() { return bIsInitialized; }

	void Tick(float InDeltaSeconds);
	/**
	 * Implementation(s)
	 */

public:
	
	rive::Artboard* GetNativeArtboard() const;

	rive::AABB GetBounds() const;

	FVector2f GetSize() const;

	UE::Rive::Core::FURStateMachine* GetStateMachine() const;

	/**
	 * Attribute(s)
	 */

private:
	void Initialize_Internal(rive::Artboard* InNativeArtboard);
	void Tick_Render(float InDeltaSeconds);
	void Tick_Statemachine(float InDeltaSeconds);
	
	UE::Rive::Renderer::IRiveRenderTargetPtr RiveRenderTarget;
	mutable bool bIsInitialized = false;

	std::unique_ptr<rive::ArtboardInstance> NativeArtboardPtr = nullptr;
	UE::Rive::Core::FURStateMachinePtr DefaultStateMachinePtr = nullptr;

#endif // WITH_RIVE
};
