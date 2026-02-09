// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveRenderer.h"

#include <rive/command_server.hpp>

#include "RenderContextRHIImpl.hpp"
#include "RiveRenderTargetRHI.h"
#include "RHICommandList.h"
#include "RenderGraphUtils.h"
#include "RenderGraphResources.h"
#include "RiveStats.h"

static const FString RiveRenderOverrideDescription =
    TEXT("Forces a specific rendering interlock mode for rive renderer.\n"
         "\tatomics: Forces atomic interlock mode\n"
         "\traster: Forces raster ordered interlock mode\n"
         "\tmsaa: Forces msaa interlock mode\n");

FRiveRenderer::FRiveRenderer() :
    CommandQueue(rive::make_rcp<rive::CommandQueue>()),
    CommandBuilder(CommandQueue)
{
    FCommandLine::RegisterArgument(TEXT("riveRenderOverride"),
                                   ECommandLineArgumentFlags::GameContexts |
                                       ECommandLineArgumentFlags::EditorContext,
                                   RiveRenderOverrideDescription);

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
    UTexture2DDynamic* InRenderTarget)
{
    check(IsInGameThread());

    const TSharedPtr<FRiveRenderTargetRHI> RiveRenderTarget =
        MakeShared<FRiveRenderTargetRHI>(this, InRiveName, InRenderTarget);

    return RiveRenderTarget;
}

TSharedPtr<FRiveRenderTarget> FRiveRenderer::CreateRenderTarget(
    const FName& InRiveName,
    UTextureRenderTarget2D* InRenderTarget)
{
    check(IsInGameThread());

    const TSharedPtr<FRiveRenderTargetRHI> RiveRenderTarget =
        MakeShared<FRiveRenderTargetRHI>(this, InRiveName, InRenderTarget);

    return RiveRenderTarget;
}

TSharedPtr<FRiveRenderTarget> FRiveRenderer::CreateRenderTarget(
    FRDGBuilder& GraphBuilder,
    const FName& InRiveName,
    FRDGTextureRef InRenderTarget)
{
    return MakeShared<FRiveRenderTargetRHI>(GraphBuilder,
                                            this,
                                            InRiveName,
                                            InRenderTarget);
    ;
}

TSharedPtr<FRiveRenderTarget> FRiveRenderer::CreateRenderTarget(
    const FName& InRiveName,
    FRenderTarget* RenderTarget)
{
    return MakeShared<FRiveRenderTargetRHI>(this, InRiveName, RenderTarget);
}

DECLARE_GPU_STAT_NAMED(CreateRenderContext,
                       TEXT("FRiveRendererRHI::CreateRenderContext"));
void FRiveRenderer::CreateRenderContext(FRHICommandListImmediate& RHICmdList)
{
    check(IsInRenderingThread());
    check(GDynamicRHI);

    SCOPED_GPU_STAT(RHICmdList, CreateRenderContext);

    if (GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Null)
    {
        return;
    }

    RenderContextRHIImpl::RHICapabilitiesOverrides Overrides;

    FString renderOverrides;
    if (FParse::Value(FCommandLine::Get(),
                      TEXT("riveRenderOverride="),
                      renderOverrides))
    {
        auto lower = renderOverrides.ToLower();
        if (lower == TEXT("atomics"))
        {
            Overrides.bSupportsPixelShaderUAVs = true;
            Overrides.bSupportsRasterOrderViews = false;
        }
        else if (lower == TEXT("raster"))
        {
            Overrides.bSupportsPixelShaderUAVs = false;
            Overrides.bSupportsRasterOrderViews = true;
        }
        else if (lower == TEXT("msaa"))
        {
            Overrides.bSupportsPixelShaderUAVs = false;
            Overrides.bSupportsRasterOrderViews = false;
        }
    }

    RenderContext = RenderContextRHIImpl::MakeContext(RHICmdList, Overrides);
}
