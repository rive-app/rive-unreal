// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRendererModule.h"

namespace UE::Rive::Renderer::Private
{
    class FRiveRendererModule : public IRiveRendererModule
    {
        //~ BEGIN : IModuleInterface Interface

    public:
        virtual void StartupModule() override;

        virtual void ShutdownModule() override;

        //~ END : IModuleInterface Interface

        //~ BEGIN : IRiveRendererModule Interface

    public:

        virtual IRiveRenderer* GetRenderer() override;
        //~ END : IRiveRendererModule Interface

        virtual void CallOrRegister_OnRendererInitialized(FSimpleMulticastDelegate::FDelegate&& Delegate) override;
        
        /**
         * Attribute(s)
         */

    private:
        TSharedPtr<IRiveRenderer> RiveRenderer;
        FSimpleMulticastDelegate OnRendererInitializedDelegate;
        FDelegateHandle OnBeginFrameHandle;
    };
}