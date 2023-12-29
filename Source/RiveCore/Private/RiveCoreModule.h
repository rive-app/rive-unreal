// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IRiveCoreModule.h"

namespace UE::Rive::Core::Private
{
    class FRiveCoreModule final : public IRiveCoreModule
    {
        //~ BEGIN : IModuleInterface Interface

    public:

        void StartupModule() override;

        void ShutdownModule() override;

        //~ END : IModuleInterface Interface
    };
}