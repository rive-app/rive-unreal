// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveCommandBuilder.h"

#if WITH_RIVE

struct FRiveDescriptor;
struct FRiveCommandBuilder;
#include "Input/Events.h"
#include "Layout/Geometry.h"

THIRD_PARTY_INCLUDES_START
#include "rive/command_queue.hpp"
#include "rive/event_report.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

class URiveViewModel;

/**
 * Represents a Rive State Machine from an Artboard. A State Machine contains
 * Inputs.
 */
struct RIVE_API FRiveStateMachine : public TSharedFromThis<FRiveStateMachine>
{
    bool IsValid() const
    {
        return NativeStateMachineHandle != RIVE_NULL_HANDLE;
    }

    void Destroy(FRiveCommandBuilder& CommandBuilder);
    void Initialize(FRiveCommandBuilder& CommandBuilder,
                    rive::ArtboardHandle InOwningArtboardHandle,
                    const FString& StateMachineName);

    void Advance(FRiveCommandBuilder&, float InSeconds);

    uint32 GetInputCount() const;

    void FireTrigger(const FString& InPropertyName) const;

    bool PointerDown(const FGeometry& MyGeometry,
                     const FRiveDescriptor& InDescriptor,
                     const FPointerEvent& MouseEvent);

    bool PointerMove(const FGeometry& MyGeometry,
                     const FRiveDescriptor& InDescriptor,
                     const FPointerEvent& MouseEvent);

    bool PointerUp(const FGeometry& MyGeometry,
                   const FRiveDescriptor& InDescriptor,
                   const FPointerEvent& MouseEvent);

    bool PointerExit(const FGeometry& InGeometry,
                     const FRiveDescriptor& InDescriptor,
                     const FPointerEvent& MouseEvent);

    void BindViewModel(TObjectPtr<URiveViewModel> ViewModel);

    void SetStateMachineSettled(bool inStateMachineSettled);

    bool IsStateMachineSettled() const { return bStateMachineSettled; }

    const FString& GetStateMachineName() const { return StateMachineName; }

    rive::StateMachineHandle GetNativeStateMachineHandle() const
    {
        return NativeStateMachineHandle;
    }

    TArray<FString> BoolInputNames;
    TArray<FString> NumberInputNames;
    TArray<FString> TriggerInputNames;

private:
    FString StateMachineName;
    rive::StateMachineHandle NativeStateMachineHandle = RIVE_NULL_HANDLE;
    static rive::EventReport NullEvent;
    bool bStateMachineSettled = false;
};
