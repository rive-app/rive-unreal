// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IRiveCoreModule.h"
#include "Core/UnrealRiveFactory.h"

namespace UE::Rive::Core::Private
{
    class FRiveCoreModule final : public IRiveCoreModule
    {
        //~ BEGIN : IModuleInterface Interface

    public:

        void StartupModule() override;

        void ShutdownModule() override;

        //~ END : IModuleInterface Interface


        //~ BEGIN : IRiveCoreModule Interface

    public:

        virtual FUnrealRiveFactory& GetRiveFactory() override;

        //~ END : IRiveCoreModule Interface

        /**
         * Attribute(s)
         */

    private:

        FUnrealRiveFactoryPtr UnrealRiveFactory;
    };
}