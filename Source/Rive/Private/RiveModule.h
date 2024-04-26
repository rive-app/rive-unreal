// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveModule.h"

namespace UE { namespace Rive { namespace Private
{
    class FRiveModule final : public IRiveModule
    {
        //~ BEGIN : IModuleInterface Interface

    public:

        virtual void StartupModule() override;

        virtual void ShutdownModule() override;

        //~ END : IModuleInterface Interface

        /**
         * Implementation(s)
         */

    private:

        void TestRiveIntegration();
    };
}}}
