// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Core/URStateMachine.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/file.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

namespace UE::Rive::Core
{
    class FURArtboard;

    /**
     * Type definition for unique pointer reference to the instance of FURArtboard.
     */
    using FURArtboardPtr = TUniquePtr<FURArtboard>;

    /**
     * Represents a Rive Artboard with a File. An Artboard contains State Machines and Animations.
     */
    class RIVE_API FURArtboard
    {
        /**
         * Structor(s)
         */

    public:

        FURArtboard() = default;

        ~FURArtboard() = default;

#if WITH_RIVE

        explicit FURArtboard(rive::File* InNativeFilePtr);

        /**
         * Implementation(s)
         */

    public:

        // Just for testing
        rive::Artboard* GetNativeArtboard() const;

        rive::AABB GetBounds() const;

        FVector2f GetSize() const;

        Core::FURStateMachine* GetStateMachine() const;

        /**
         * Attribute(s)
         */

    private:

        std::unique_ptr<rive::ArtboardInstance> NativeArtboardPtr = nullptr;

        Core::FURStateMachinePtr DefaultStateMachinePtr = nullptr;

#endif // WITH_RIVE
    };
}
