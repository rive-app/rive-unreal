// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererModule.h"
#include "RiveRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Logs/RiveRendererLog.h"
#include "Interfaces/IPluginManager.h"

#if PLATFORM_WINDOWS
#include "Platform/RiveRendererD3D11.h"
#elif PLATFORM_APPLE
#include "Platform/RiveRendererMetal.h"
#elif PLATFORM_ANDROID
#include "Platform/RiveRendererOpenGL.h"
#endif // PLATFORM_WINDOWS

#define LOCTEXT_NAMESPACE "RiveRendererModule"

void UE::Rive::Renderer::Private::FRiveRendererModule::StartupModule()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    
    LoadDll();
    
    check(GDynamicRHI);
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
#if PLATFORM_APPLE
        case ERHIInterfaceType::Metal:
        {
            RiveRenderer = MakeShared<FRiveRendererMetal>();
            
            break;
        }
#endif // PLATFORM_APPLE
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
    // Add on the relative location of the third party dll and load it
#if WITH_EDITOR
    // In Editor, we don't want to load the dll from Binaries as it will lock the dll
    // and the packaging will sometimes fail as UE is trying to replace the dll
    // Get the base directory of this plugin
    const FString BaseDir = IPluginManager::Get().FindPlugin("Rive")->GetBaseDir();
    const FString RiveHarfbuzzLibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/RiveLibrary/Libraries/Win64/rive_harfbuzz.dll"));
#else
    const FString RiveHarfbuzzLibraryPath = TEXT("rive_harfbuzz.dll");
#endif
    
    {
        RiveHarfbuzzDllHandle = FPlatformProcess::GetDllHandle(*RiveHarfbuzzLibraryPath);
        if (RiveHarfbuzzDllHandle != nullptr)
        {
            RIVE_DEBUG_VERBOSE("Loaded RiveHarfbuzz from '%s'", *RiveHarfbuzzLibraryPath);
        }
        else
        {
            UE_LOG(LogRiveRenderer, Error, TEXT("Unable to load RiveHarfbuzz from '%s'"), *RiveHarfbuzzLibraryPath);
            return false;
        }
    }
#elif PLATFORM_ANDROID
    // Add on the relative location of the third party dll and load it
    const FString RiveHarfbuzzLibraryPath = TEXT("librive_harfbuzz.so");
    
    FModuleManager::Get().LoadModule(TEXT("OpenGLDrv"));
    {
        RiveHarfbuzzDllHandle = FPlatformProcess::GetDllHandle(*RiveHarfbuzzLibraryPath);
        if (RiveHarfbuzzDllHandle != nullptr)
        {
            RIVE_DEBUG_VERBOSE("Loaded RiveHarfbuzz from '%s'", *RiveHarfbuzzLibraryPath);
        }
        else
        {
            UE_LOG(LogRiveRenderer, Error, TEXT("Unable to load RiveHarfbuzz from '%s'"), *RiveHarfbuzzLibraryPath);
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
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Renderer::Private::FRiveRendererModule, RiveRenderer)
