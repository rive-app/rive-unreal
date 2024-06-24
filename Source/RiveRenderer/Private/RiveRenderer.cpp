// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderer.h"

#include "Async/Async.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Logs/RiveRendererLog.h"
#include "RenderingThread.h"
#include "TextureResource.h"
#include "UObject/Package.h"

#include "rive/pls/pls_render_context.hpp"

FRiveRenderer::FRiveRenderer()
{
    RIVE_DEBUG_FUNCTION_INDENT;
}

FRiveRenderer::~FRiveRenderer()
{
    RIVE_DEBUG_FUNCTION_INDENT;
    InitializationState = ERiveInitState::Deinitializing;

    if (!IsRunningCommandlet())
    {

        if (PLSRenderContext)
        {
            FScopeLock Lock(&ThreadDataCS);
            PLSRenderContext->releaseResources();
        }
    }

    FlushRenderingCommands();
}

void FRiveRenderer::Initialize()
{
    check(IsInGameThread());
    
    FScopeLock Lock(&ThreadDataCS);
    if (InitializationState != ERiveInitState::Uninitialized)
    {
        return;
    }
    InitializationState = ERiveInitState::Initializing;
    
    ENQUEUE_RENDER_COMMAND(FRiveRenderer_Initialize)(
    [this](FRHICommandListImmediate& RHICmdList)
    {
        CreatePLSContext_RenderThread(RHICmdList);
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            {
                FScopeLock Lock(&ThreadDataCS);
                InitializationState = ERiveInitState::Initialized;
            }
            OnInitializedDelegate.Broadcast(this);
        });
    });
}

#if WITH_RIVE

void FRiveRenderer::CallOrRegister_OnInitialized(FOnRendererInitialized::FDelegate&& Delegate)
{
    ThreadDataCS.Lock();
    const bool bIsInitialized = IsInitialized();
    ThreadDataCS.Unlock();
    
    if (bIsInitialized)
    {
        Delegate.Execute(this);
    }
    else
    {
        OnInitializedDelegate.Add(MoveTemp(Delegate));
    }
}

rive::pls::PLSRenderContext* FRiveRenderer::GetPLSRenderContextPtr()
{
    if (!PLSRenderContext)
    {
        UE_LOG(LogRiveRenderer, Error, TEXT("Rive PLS Render Context is uninitialized."));
        return nullptr;
    }

    return PLSRenderContext.get();
}


#endif // WITH_RIVE

UTextureRenderTarget2D* FRiveRenderer::CreateDefaultRenderTarget(FIntPoint InTargetSize)
{
    UTextureRenderTarget2D* const RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());

    RenderTarget->OverrideFormat = EPixelFormat::PF_R8G8B8A8;
    RenderTarget->bCanCreateUAV = true;
    RenderTarget->ResizeTarget(InTargetSize.X, InTargetSize.Y);
    RenderTarget->UpdateResourceImmediate(true);

    FlushRenderingCommands();

    return RenderTarget;
}
