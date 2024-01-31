// Copyright Rive, Inc. All rights reserved.

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

        TSharedPtr<IRiveRenderer> RiveRenderer;
    };
}