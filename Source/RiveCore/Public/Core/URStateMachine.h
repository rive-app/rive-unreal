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
    class RIVECORE_API FURStateMachine
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

        uint32 GetNumberValue(const FString& InPropertyName) const;

        void SetBoolValue(const FString& InPropertyName, bool bNewValue);

        void SetNumberValue(const FString& InPropertyName, uint32 NewValue);

        bool OnMouseButtonDown(const FVector2f& NewPosition);

        bool OnMouseMove(const FVector2f& NewPosition);

        bool OnMouseButtonUp(const FVector2f& NewPosition);

        /**
         * Attribute(s)
         */

    private:

        std::unique_ptr<rive::StateMachineInstance> NativeStateMachinePtr = nullptr;

#endif // WITH_RIVE
    };
}
