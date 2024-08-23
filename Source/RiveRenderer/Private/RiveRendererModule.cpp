// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererModule.h"
#include "RiveRenderer.h"
#include "Logs/RiveRendererLog.h"
#include "Platform/RiveRendererRHI.h"

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
    
    UE_LOG(LogRiveRenderer, Display, TEXT("Rive running on RHI 'RHI'"))
    RiveRenderer = MakeShared<FRiveRendererRHI>();
    if(!IsRunningCommandlet())
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

void FRiveRendererModule::ShutdownModule()
{
    if (RiveRenderer)
    {
        RiveRenderer.Reset();
    }
}

IRiveRenderer* FRiveRendererModule::GetRenderer()
{
    return RiveRenderer.Get();
}

void FRiveRendererModule::CallOrRegister_OnRendererInitialized(FSimpleMulticastDelegate::FDelegate&& Delegate)
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

IMPLEMENT_MODULE(FRiveRendererModule, RiveRenderer)
