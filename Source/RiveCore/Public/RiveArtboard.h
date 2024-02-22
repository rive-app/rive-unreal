// Copyright Rive, Inc. All rights reserved.
#pragma once
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

#if WITH_RIVE

	void Initialize(rive::File* InNativeFilePtr);
	void Initialize(rive::File* InNativeFilePtr, int32 InIndex, const FString& InStateMachineName = TEXT_EMPTY);
	void Initialize(rive::File* InNativeFilePtr, const FString& InName, const FString& InStateMachineName = TEXT_EMPTY);

	bool IsInitialized() { return bIsInitialized; }
	/**
	 * Implementation(s)
	 */

public:
	// Just for testing
	rive::Artboard* GetNativeArtboard() const;

	rive::AABB GetBounds() const;

	FVector2f GetSize() const;

	UE::Rive::Core::FURStateMachine* GetStateMachine() const;

	/**
	 * Attribute(s)
	 */

private:
	bool bIsInitialized = false;

	std::unique_ptr<rive::ArtboardInstance> NativeArtboardPtr = nullptr;

	UE::Rive::Core::FURStateMachinePtr DefaultStateMachinePtr = nullptr;

#endif // WITH_RIVE
};
