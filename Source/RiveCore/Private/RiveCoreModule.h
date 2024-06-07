// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveCoreModule.h"

namespace UE::Rive::Core::Private
{
    class FRiveCoreModule final : public IRiveCoreModule
    {
        //~ BEGIN : IModuleInterface Interface

    public:

        virtual void StartupModule() override;

        virtual void ShutdownModule() override;

        //~ END : IModuleInterface Interface
    };
}
