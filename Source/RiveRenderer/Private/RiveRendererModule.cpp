// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererModule.h"
#include "RiveRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Logs/RiveRendererLog.h"
#include "Platform/RiveRendererOpenGL.h"

#if PLATFORM_WINDOWS
#include "Platform/RiveRendererD3D11.h"
#endif // PLATFORM_WINDOWS

#define LOCTEXT_NAMESPACE "RiveRendererModule"

void UE::Rive::Renderer::Private::FRiveRendererModule::StartupModule()
{
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
            UE_LOG(LogRiveRenderer, Warning, TEXT("RHIGetInterfaceType returned 'OpenGL'"))
            UE_LOG(LogTemp, Error, TEXT("---OPEN GL---"))
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


    FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([this]()
    {
        if (RiveRenderer.IsValid())
        {
            RiveRenderer->Initialize();
            OnRendererInitializedDelegate.Broadcast();
        }
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
#if PLATFORM_ANDROID
    FModuleManager::Get().LoadModule(TEXT("OpenGLDrv"));
    {
        const FString WwisePath = FPaths::ProjectDir() / FString::Printf(TEXT("Binaries/Android/"));
        FString Path = TEXT("librive_harfbuzz.so");
        RiveHarfbuzzDllHandle = FPlatformProcess::GetDllHandle(*Path);
        if (RiveHarfbuzzDllHandle != nullptr)
        {
            UE_LOG(LogRiveRenderer, Warning, TEXT("Loaded RiveHarfbuzz from '%s'"), *Path);
        }
        else
        {
            UE_LOG(LogRiveRenderer, Error, TEXT("Unable to load RiveHarfbuzz from '%s'"), *Path);
            return false;
        }
    }
    {
        FString Path = TEXT("libGLESv2.so");
        libGLESv2DllHandle = FPlatformProcess::GetDllHandle(*Path);
        if (libGLESv2DllHandle != nullptr)
        {
            UE_LOG(LogRiveRenderer, Warning, TEXT("Loaded libGLESv2 from '%s'"), *Path);
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
#if PLATFORM_ANDROID
    if (RiveHarfbuzzDllHandle)
    {
        UE_LOG(LogRiveRenderer, Warning, TEXT("Releasing RiveHarfbuzz Dll..."));
        FPlatformProcess::FreeDllHandle(RiveHarfbuzzDllHandle);
        RiveHarfbuzzDllHandle = nullptr;
    }
    if (libGLESv2DllHandle)
    {
        UE_LOG(LogRiveRenderer, Warning, TEXT("Releasing libGLESv2 Dll..."));
        FPlatformProcess::FreeDllHandle(libGLESv2DllHandle);
        libGLESv2DllHandle = nullptr;
    }
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Renderer::Private::FRiveRendererModule, RiveRenderer)
