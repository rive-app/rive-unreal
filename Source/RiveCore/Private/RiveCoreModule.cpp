// Copyright Epic Games, Inc. All Rights Reserved.

#include "RiveCoreModule.h"

#define LOCTEXT_NAMESPACE "RiveCore"

void UE::Rive::Core::Private::FRiveCoreModule::StartupModule()
{
}

void UE::Rive::Core::Private::FRiveCoreModule::ShutdownModule()
{
}

UE::Rive::Core::FUnrealRiveFactory& UE::Rive::Core::Private::FRiveCoreModule::GetRiveFactory()
{
    if (!UnrealRiveFactory.IsValid())
    {
        UnrealRiveFactory = MakeUnique<UE::Rive::Core::FUnrealRiveFactory>();
    }

    return *UnrealRiveFactory.Get();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Core::Private::FRiveCoreModule, RiveCore)
