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

        void CallOrRegister_OnRendererInitialized(FSimpleMulticastDelegate::FDelegate&& Delegate);
        
        /**
         * Attribute(s)
         */

    private:
        bool LoadDll();
        void ReleaseDll();
#if PLATFORM_ANDROID
        void* RiveHarfbuzzDllHandle = nullptr;
        void* libGLESv2DllHandle = nullptr;
#endif

        TSharedPtr<IRiveRenderer> RiveRenderer;
        FSimpleMulticastDelegate OnRendererInitializedDelegate;
    };
}