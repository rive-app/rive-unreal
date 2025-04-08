// Copyright Rive, Inc. All rights reserved.
#pragma once

#include "CoreMinimal.h"

#if WITH_RIVE

THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/animation/state_machine_instance.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

class URiveViewModelInstance;

/**
 * Represents a Rive State Machine from an Artboard. A State Machine contains
 * Inputs.
 */
class RIVE_API FRiveStateMachine
{
    /**
     * Structor(s)
     */

public:
    FRiveStateMachine() = default;

    ~FRiveStateMachine() = default;

    const std::unique_ptr<rive::StateMachineInstance>&
    GetNativeStateMachinePtr() const
    {
        return NativeStateMachinePtr;
    }

    bool IsValid() const { return NativeStateMachinePtr != nullptr; }
#if WITH_RIVE

    explicit FRiveStateMachine(rive::ArtboardInstance* InNativeArtboardInst,
                               const FString& InStateMachineName);

    /**
     * Implementation(s)
     */

public:
    bool Advance(float InSeconds);

    uint32 GetInputCount() const;

    rive::SMIInput* GetInput(uint32 AtIndex) const;

    void FireTrigger(const FString& InPropertyName) const;

    bool GetBoolValue(const FString& InPropertyName) const;

    float GetNumberValue(const FString& InPropertyName) const;

    void SetBoolValue(const FString& InPropertyName, bool bNewValue);

    void SetNumberValue(const FString& InPropertyName, float NewValue);

    bool PointerDown(const FVector2f& NewPosition);

    bool PointerMove(const FVector2f& NewPosition);

    bool PointerUp(const FVector2f& NewPosition);

    bool PointerExit(const FVector2f& NewPosition);

    const rive::EventReport GetReportedEvent(int32 AtIndex) const;

    int32 GetReportedEventsCount() const;

    bool HasAnyReportedEvents() const;

    void SetViewModelInstance(URiveViewModelInstance* RuntimeInstance);

    /**
     * Attribute(s)
     */
    const FString& GetStateMachineName() const { return StateMachineName; }

    TArray<FString> BoolInputNames;
    TArray<FString> NumberInputNames;
    TArray<FString> TriggerInputNames;

private:
    FString StateMachineName;

    std::unique_ptr<rive::StateMachineInstance> NativeStateMachinePtr = nullptr;

    static rive::EventReport NullEvent;

#endif // WITH_RIVE
};
