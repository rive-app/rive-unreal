// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RiveRenderer.h"

#include "Async/Async.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Logs/RiveRendererLog.h"
#include "RenderingThread.h"
#include "TextureResource.h"
#include "rive/command_server.hpp"
#include "UObject/Package.h"
#include "RiveStats.h"
#include "Platform/RenderContextRHIImpl.hpp"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/render_context.hpp"
THIRD_PARTY_INCLUDES_END

FRiveRenderer::FRiveRenderer() :
    CommandQueue(rive::make_rcp<rive::CommandQueue>()),
    CommandBuilder(CommandQueue)
{
    RIVE_DEBUG_FUNCTION_INDENT;

    OnBeingFrameGameThreadHandle = FCoreDelegates::OnBeginFrame.AddRaw(
        this,
        &FRiveRenderer::BeginFrameGameThread);
    OnEndFrameGameThreadHandle =
        FCoreDelegates::OnEndFrame.AddRaw(this,
                                          &FRiveRenderer::EndFrameGameThread);

    ENQUEUE_RENDER_COMMAND(FRiveRenderer_Initialize)
    ([this](FRHICommandListImmediate& RHICmdList) {
        CreateRenderContext(RHICmdList);
        check(RenderContext);
        CommandServer =
            MakeUnique<rive::CommandServer>(CommandQueue, RenderContext.get());
        OnBeingFrameRenderThreadHandle = FCoreDelegates::OnBeginFrameRT.AddRaw(
            this,
            &FRiveRenderer::BeginFrameRenderThread);
    });
}

FRiveRenderer::~FRiveRenderer()
{
    RIVE_DEBUG_FUNCTION_INDENT;

    if (!IsRunningCommandlet())
    {
        if (RenderContext)
        {
            RenderContext->releaseResources();
        }
    }

    CommandQueue->disconnect();

    FlushRenderingCommands();
    FCoreDelegates::OnBeginFrame.Remove(OnBeingFrameGameThreadHandle);
    FCoreDelegates::OnBeginFrame.Remove(OnEndFrameGameThreadHandle);
    FCoreDelegates::OnBeginFrameRT.Remove(OnBeingFrameRenderThreadHandle);
}

DECLARE_GPU_STAT_NAMED(BeingFrameRenderThread,
                       TEXT("FRiveRenderer::BeingFrameRenderThread"));
void FRiveRenderer::BeginFrameRenderThread()
{
    SCOPED_GPU_STAT(GRHICommandList.GetImmediateCommandList(),
                    BeingFrameRenderThread);

    check(IsInRenderingThread());
    check(CommandServer);
#if WITH_EDITOR
    static const auto CVarInterlock =
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.rive.interlock"));
    static int32 LastCVar = 0;
    int32 CVar = CVarInterlock->GetInt();
    if (LastCVar != CVar)
    {
        LastCVar = CVar;
        if (auto impl = RenderContext->static_impl_cast<RenderContextRHIImpl>())
        {
            impl->updateFromInterlockCVar(CVar);
        }
    }
#endif

    SCOPED_NAMED_EVENT_TEXT(TEXT("CommandServer->processCommands"),
                            FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("CommandServer->processCommands"),
                                STAT_COMMANDSERVER_PROCESSCOMMANDS,
                                STATGROUP_Rive);

    CommandServer->processCommands();
}

void FRiveRenderer::BeginFrameGameThread()
{
    check(IsInGameThread());
    check(CommandQueue);

    SCOPED_NAMED_EVENT_TEXT(TEXT("CommandQueue->processMessages"),
                            FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("CommandQueue->processMessages"),
                                STAT_COMMANDQUEUE_PROCESSMESSAGES,
                                STATGROUP_Rive);

    CommandBuilder.Reset();
    CommandQueue->processMessages();
}

void FRiveRenderer::EndFrameGameThread()
{
    check(IsInGameThread());
    check(CommandQueue);

    SCOPED_NAMED_EVENT_TEXT(TEXT("CommandBuilder.Execute"), FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("CommandBuilder.Execute"),
                                STAT_RIVECOMMANDBUILDER_EXECUTE,
                                STATGROUP_Rive);

    CommandBuilder.Execute();
}

rive::gpu::RenderContext* FRiveRenderer::GetRenderContext()
{
    check(IsInRenderingThread());
    check(RenderContext);
    return RenderContext.get();
}

TSharedPtr<FRiveRenderTarget> FRiveRenderer::CreateRenderTarget(
    const FName& InRiveName,
    FRenderTarget* RenderTarget)
{
    UE_LOG(LogRiveRenderer,
           Error,
           TEXT("CreateRenderTarget with FRenderTarget is not supported"));
    return nullptr;
}

TSharedPtr<FRiveRenderTarget> FRiveRenderer::CreateRenderTarget(
    FRDGBuilder& GraphBuilder,
    const FName& InRiveName,
    FRDGTextureRef InRenderTarget)
{
    UE_LOG(LogRiveRenderer,
           Error,
           TEXT("CreateRenderTarget with FRDGTexture is not supported"));
    return nullptr;
}
