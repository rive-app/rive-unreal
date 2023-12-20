// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IRiveRendererModule.h"

namespace UE::Rive::Renderer::Private
{
    class FRiveRendererModule : public IRiveRendererModule
    {
        //~ BEGIN : IModuleInterface Interface

    public:

        void StartupModule() override;

        void ShutdownModule() override;

        //~ END : IModuleInterface Interface

        //~ BEGIN : IRiveRendererModule Interface

    public:

        virtual IRiveRenderer* GetRenderer() override;

        //~ END : IRiveRendererModule Interface

        /**
         * Attribute(s)
         */

    private:

        TUniquePtr<IRiveRenderer> RiveRenderer;
    };
}