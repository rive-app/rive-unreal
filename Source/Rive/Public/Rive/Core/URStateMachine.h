// Copyright Rive, Inc. All rights reserved.

#pragma once

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/animation/state_machine_instance.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Core
{
    class FURStateMachine;

    /**
     * Type definition for unique pointer reference to the instance of FURStateMachine.
     */
    using FURStateMachinePtr = TUniquePtr<FURStateMachine>;

    /**
     * Represents a Rive State Machine from an Artboard. A State Machine contains Inputs.
     */
    class RIVE_API FURStateMachine
    {
        /**
         * Structor(s)
         */

    public:

        FURStateMachine() = default;

        ~FURStateMachine() = default;

#if WITH_RIVE

        explicit FURStateMachine(rive::ArtboardInstance* InNativeArtboardInst);

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

        bool OnMouseButtonDown(const FVector2f& NewPosition);

        bool OnMouseMove(const FVector2f& NewPosition);

        bool OnMouseButtonUp(const FVector2f& NewPosition);

        const rive::EventReport GetReportedEvent(int32 AtIndex) const;

        int32 GetReportedEventsCount() const;

        bool HasAnyReportedEvents() const;

        /**
         * Attribute(s)
         */

    private:

        std::unique_ptr<rive::StateMachineInstance> NativeStateMachinePtr = nullptr;

        static rive::EventReport NullEvent;

#endif // WITH_RIVE
    };
}
