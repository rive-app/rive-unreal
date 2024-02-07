// Copyright Rive, Inc. All rights reserved.

#include "Rive/Core/URArtboard.h"
#include "Logs/RiveLog.h"
#include "Rive/Core/URStateMachine.h"

#if WITH_RIVE

UE::Rive::Core::FURArtboard::FURArtboard(rive::File* InNativeFilePtr)
{
    if (!InNativeFilePtr) return;
    
    if (rive::Artboard* NativeArtboard = InNativeFilePtr->artboard())
    {
        NativeArtboardPtr = NativeArtboard->instance();

        NativeArtboardPtr->advance(0);
     
        DefaultStateMachinePtr = MakeUnique<Core::FURStateMachine>(NativeArtboardPtr.get());
    }
}

rive::Artboard* UE::Rive::Core::FURArtboard::GetNativeArtboard() const
{
    if (!NativeArtboardPtr)
    {
        UE_LOG(LogRive, Error, TEXT("Could not retrieve artboard as we have detected an empty rive atrboard."));

        return nullptr;
    }

    return NativeArtboardPtr.get();
}

rive::AABB UE::Rive::Core::FURArtboard::GetBounds() const
{
    if (!NativeArtboardPtr)
    {
        UE_LOG(LogRive, Error, TEXT("Could not retrieve artboard bounds as we have detected an empty rive atrboard."));

        return { 0, 0, 0, 0 };
    }

    return NativeArtboardPtr->bounds();
}

FVector2f UE::Rive::Core::FURArtboard::GetSize() const
{
    if (!NativeArtboardPtr)
    {
        UE_LOG(LogRive, Error, TEXT("Could not retrieve artboard size as we have detected an empty rive atrboard."));

        return FVector2f::ZeroVector;
    }

    return { NativeArtboardPtr->width(), NativeArtboardPtr->height() };
}

UE::Rive::Core::FURStateMachine* UE::Rive::Core::FURArtboard::GetStateMachine() const
{
    if (!DefaultStateMachinePtr)
    {
        UE_LOG(LogRive, Error, TEXT("Could not retrieve state machine as we have detected an empty rive artboard."));

        return nullptr;
    }

    return DefaultStateMachinePtr.Get();
}

#endif // WITH_RIVE
