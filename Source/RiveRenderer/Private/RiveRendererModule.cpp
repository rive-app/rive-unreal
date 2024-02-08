// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererModule.h"
#include "RiveRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Logs/RiveRendererLog.h"

#if PLATFORM_WINDOWS
#include "Platform/RiveRendererD3D11.h"
#elif PLATFORM_ANDROID
#include "Platform/RiveRendererOpenGL.h"
#endif // PLATFORM_WINDOWS

#define LOCTEXT_NAMESPACE "RiveRendererModule"

void UE::Rive::Renderer::Private::FRiveRendererModule::StartupModule()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    
    // Create Platform Specific Renderer
    RiveRenderer = nullptr;
    switch (RHIGetInterfaceType())
    {
#if PLATFORM_WINDOWS
    case ERHIInterfaceType::D3D11:
        {
            RiveRenderer = MakeShared<FRiveRendererD3D11>();
            break;
        }
    case ERHIInterfaceType::D3D12:
        {
            break;
        }
#endif // PLATFORM_WINDOWS
#if PLATFORM_ANDROID
    case ERHIInterfaceType::OpenGL:
        {
            RIVE_DEBUG_VERBOSE("RHIGetInterfaceType returned 'OpenGL'")
            RiveRenderer = MakeShared<FRiveRendererOpenGL>();
            break;
        }
#endif // PLATFORM_ANDROID
    case ERHIInterfaceType::Vulkan:
        {
            break;
        }
    default:
        break;
    }
    
    // OnBeginFrameHandle = FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([this]()  // Crashes sometimes on Android when on GameThread
    OnBeginFrameHandle = FCoreDelegates::OnBeginFrame.AddLambda([this]()
    {
        if (RiveRenderer.IsValid())
        {
            RiveRenderer->Initialize();
            OnRendererInitializedDelegate.Broadcast();
        }
        FCoreDelegates::OnBeginFrame.Remove(OnBeginFrameHandle);
    });
}

void UE::Rive::Renderer::Private::FRiveRendererModule::ShutdownModule()
{
    ReleaseDll();
}

UE::Rive::Renderer::IRiveRenderer* UE::Rive::Renderer::Private::FRiveRendererModule::GetRenderer()
{
    return RiveRenderer.Get();
}

void UE::Rive::Renderer::Private::FRiveRendererModule::CallOrRegister_OnRendererInitialized(FSimpleMulticastDelegate::FDelegate&& Delegate)
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

bool UE::Rive::Renderer::Private::FRiveRendererModule::LoadDll()
{
    RIVE_DEBUG_FUNCTION_INDENT;
#if PLATFORM_WINDOWS
    //todo: check if we need to have harfbuzz as a dynamic dll for windows
    // {
    //     FString Path = TEXT("rive_harfbuzz.dll");
    //     RiveHarfbuzzDllHandle = FPlatformProcess::GetDllHandle(*Path);
    //     if (RiveHarfbuzzDllHandle != nullptr)
    //     {
    //         UE_LOG(LogRiveRenderer, Warning, TEXT("Loaded RiveHarfbuzz from '%s'"), *Path);
    //     }
    //     else
    //     {
    //         UE_LOG(LogRiveRenderer, Error, TEXT("Unable to load RiveHarfbuzz from '%s'"), *Path);
    //         return false;
    //     }
    // }
#elif PLATFORM_ANDROID
    FModuleManager::Get().LoadModule(TEXT("OpenGLDrv"));
    {
        const FString Path = TEXT("librive_harfbuzz.so");
        RiveHarfbuzzDllHandle = FPlatformProcess::GetDllHandle(*Path);
        if (RiveHarfbuzzDllHandle != nullptr)
        {
            RIVE_DEBUG_VERBOSE("Loaded RiveHarfbuzz from '%s'", *Path);
        }
        else
        {
            UE_LOG(LogRiveRenderer, Error, TEXT("Unable to load RiveHarfbuzz from '%s'"), *Path);
            return false;
        }
    }
    {
        const FString Path = TEXT("libGLESv2.so"); //todo: check if needed
        libGLESv2DllHandle = FPlatformProcess::GetDllHandle(*Path);
        if (libGLESv2DllHandle != nullptr)
        {
             RIVE_DEBUG_VERBOSE("Loaded libGLESv2 from '%s'", *Path);
        }
        else
        {
            UE_LOG(LogRiveRenderer, Error, TEXT("Unable to load libGLESv2 from '%s'"), *Path);
            return false;
        }
    }
    
#endif
    return true;
}

void UE::Rive::Renderer::Private::FRiveRendererModule::ReleaseDll()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    if (RiveHarfbuzzDllHandle)
    {
         RIVE_DEBUG_VERBOSE("Releasing RiveHarfbuzz Dll...");
        FPlatformProcess::FreeDllHandle(RiveHarfbuzzDllHandle);
        RiveHarfbuzzDllHandle = nullptr;
    }
#if PLATFORM_ANDROID
    if (libGLESv2DllHandle)
    {
        RIVE_DEBUG_VERBOSE("Releasing libGLESv2 Dll...");
        FPlatformProcess::FreeDllHandle(libGLESv2DllHandle);
        libGLESv2DllHandle = nullptr;
    }
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Renderer::Private::FRiveRendererModule, RiveRenderer)
