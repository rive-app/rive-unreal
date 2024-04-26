// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererModule.h"
#include "RiveRenderer.h"
#include "Logs/RiveRendererLog.h"

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
    
    check(GDynamicRHI);
    // Create Platform Specific Renderer
    RiveRenderer = nullptr;
    
    const FString& RHIName = GDynamicRHI->GetName();
    bool bWasRHIHandled = false;

#if PLATFORM_WINDOWS
    if (RHIName == FString(TEXT("D3D11")))
    {
        UE_LOG(LogRiveRenderer, Display, TEXT("Rive running on RHI 'D3D11'"))
        RiveRenderer = MakeShared<FRiveRendererD3D11>();
        bWasRHIHandled = true;
    }
    if (RHIName == FString(TEXT("D3D12")))
    {
        UE_LOG(LogRiveRenderer, Error, TEXT("Rive is NOT compatible with RHI 'D3D12'"))
        bWasRHIHandled = true;
    }
#endif // PLATFORM_WINDOWS
#if PLATFORM_ANDROID
    if (RHIName == FString(TEXT("OpenGL")))
    {
        UE_LOG(LogRiveRenderer, Display, TEXT("Rive running on RHI 'OpenGL'"))
        RiveRenderer = MakeShared<FRiveRendererOpenGL>();
        bWasRHIHandled = true;
    }
#endif // PLATFORM_ANDROID
#if PLATFORM_APPLE
    if (RHIName == FString(TEXT("Metal")))
    {
        UE_LOG(LogRiveRenderer, Display, TEXT("Rive running on RHI 'Metal'"))
#if defined(WITH_RIVE_MAC_ARM64)
        UE_LOG(LogRiveRenderer, Display, TEXT("Rive running on a Mac M1/M2 processor (Arm64)"))
#elif defined(WITH_RIVE_MAC_INTEL)
        UE_LOG(LogRiveRenderer, Display, TEXT("Rive running on a Mac Intel processor (x86 x64)"))
#endif
        RiveRenderer = MakeShared<FRiveRendererMetal>();
        bWasRHIHandled = true;
    }
#endif // PLATFORM_APPLE
    if (RHIName == FString(TEXT("Vulkan")))
    {
        UE_LOG(LogRiveRenderer, Error, TEXT("Rive is NOT compatible with RHI 'Vulkan'"))
        bWasRHIHandled = true;
    }
    if(!IsRunningCommandlet() && !bWasRHIHandled)
    {
        UE_LOG(LogRiveRenderer, Error, TEXT("Rive is NOT compatible with the current unknown RHI"))
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
    if (RiveRenderer)
    {
        RiveRenderer.Reset();
    }
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(UE::Rive::Renderer::Private::FRiveRendererModule, RiveRenderer)
