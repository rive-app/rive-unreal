// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererModule.h"

#include "RiveRenderer.h"
#include "Logs/RiveRendererLog.h"
#include "Platform/RiveRendererRHI.h"
#include "RiveRendererSettings.h"

#if PLATFORM_WINDOWS
#include "Platform/RiveRendererD3D11.h"
#elif PLATFORM_APPLE
#include "Platform/RiveRendererMetal.h"
#elif PLATFORM_ANDROID
#include "Platform/RiveRendererOpenGL.h"
#endif // PLATFORM_WINDOWS

#define LOCTEXT_NAMESPACE "RiveRendererModule"

void FRiveRendererModule::StartupModule()
{
    RIVE_DEBUG_FUNCTION_INDENT;

    check(GDynamicRHI);
    // Create Platform Specific Renderer
    RiveRenderer = nullptr;

    const URiveRendererSettings* PluginSettings =
        GetDefault<URiveRendererSettings>();
    if (PluginSettings->bEnableRHITechPreview)
    {
        UE_LOG(LogRiveRenderer,
               Warning,
               TEXT("Rive running on RHI Native Tech Preview !"))
        RiveRenderer = MakeShared<FRiveRendererRHI>();
    }
    else
    {
        StartupLegacyRiveRenderer();
    }

    // OnBeginFrameHandle =
    // FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([this]()  // Crashes
    // sometimes on Android when on GameThread
    OnBeginFrameHandle = FCoreDelegates::OnBeginFrame.AddLambda([this]() {
        if (RiveRenderer.IsValid())
        {
            RiveRenderer->Initialize();
            OnRendererInitializedDelegate.Broadcast();
        }
        FCoreDelegates::OnBeginFrame.Remove(OnBeginFrameHandle);
    });
}

void FRiveRendererModule::ShutdownModule()
{
    if (RiveRenderer)
    {
        RiveRenderer.Reset();
    }
}

IRiveRenderer* FRiveRendererModule::GetRenderer() { return RiveRenderer.Get(); }

void FRiveRendererModule::CallOrRegister_OnRendererInitialized(
    FSimpleMulticastDelegate::FDelegate&& Delegate)
{
    if (RiveRenderer.IsValid() && RiveRenderer->IsInitialized())
    {
        Delegate.Execute();
    }
    else
    {
        OnRendererInitializedDelegate.Add(MoveTemp(Delegate));
    }
}

void FRiveRendererModule::StartupLegacyRiveRenderer()
{
    switch (RHIGetInterfaceType())
    {
#if PLATFORM_WINDOWS
        case ERHIInterfaceType::D3D11:
        {
            UE_LOG(LogRiveRenderer,
                   Display,
                   TEXT("Rive running on RHI 'D3D11'"))
            RiveRenderer = MakeShared<FRiveRendererD3D11>();
            break;
        }
        case ERHIInterfaceType::D3D12:
        {
            UE_LOG(LogRiveRenderer,
                   Error,
                   TEXT("Rive is NOT compatible with RHI 'D3D12'"))
            break;
        }
#endif // PLATFORM_WINDOWS
#if PLATFORM_ANDROID
        case ERHIInterfaceType::OpenGL:
        {
            UE_LOG(LogRiveRenderer,
                   Display,
                   TEXT("Rive running on RHI 'OpenGL'"))
            RiveRenderer = MakeShared<FRiveRendererOpenGL>();
            break;
        }
#endif // PLATFORM_ANDROID
#if PLATFORM_APPLE
        case ERHIInterfaceType::Metal:
        {
            UE_LOG(LogRiveRenderer,
                   Display,
                   TEXT("Rive running on RHI 'Metal'"))
#if defined(WITH_RIVE_MAC_ARM64)
            UE_LOG(LogRiveRenderer,
                   Display,
                   TEXT("Rive running on a Mac M1/M2 processor (Arm64)"))
#elif defined(WITH_RIVE_MAC_INTEL)
            UE_LOG(LogRiveRenderer,
                   Display,
                   TEXT("Rive running on a Mac Intel processor (x86 x64)"))
#endif
            RiveRenderer = MakeShared<FRiveRendererMetal>();
            break;
        }
#endif // PLATFORM_APPLE
        case ERHIInterfaceType::Vulkan:
        {
            UE_LOG(LogRiveRenderer,
                   Error,
                   TEXT("Rive is NOT compatible with RHI 'Vulkan'"))
            break;
        }
        default:
            if (!IsRunningCommandlet())
            {
                UE_LOG(
                    LogRiveRenderer,
                    Error,
                    TEXT("Rive is NOT compatible with the current unknown RHI"))
            }
            break;
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRiveRendererModule, RiveRenderer)
