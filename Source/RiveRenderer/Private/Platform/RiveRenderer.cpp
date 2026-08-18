// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveRenderer.h"

#include <rive/command_server.hpp>
#include <rive/async/work_pool.hpp>
#include "Ore/RiveOrderShaderHandler.h"
#include "RenderContextRHIImpl.hpp"
#include "RiveRenderTargetRHI.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderGraphResources.h"
#include "RiveStats.h"

// Replays a recorded frame against the real render context. The host opens
// the screen through BeginScreen and hands back a raw renderer that has to
// survive the replay, so the sink keeps the one it made.
class FRiveHostFrameSink final : public rive::cmd::HostFrameSink
{
public:
    FRiveHostFrameSink(rive::gpu::RenderContext* InRenderContext,
                       TFunctionRef<TUniquePtr<rive::Renderer>()> InBeginScreen,
                       bool bInClear,
                       uint32 InClearColor,
                       bool bInReplayOre) :
        HostFrameSink(bInClear, InClearColor, /*target=*/0, bInReplayOre),
        RenderContextPtr(InRenderContext),
        BeginScreen(InBeginScreen)
    {
        // One session serves every render target this context drives, and a
        // draw records and replays before the next one starts, so they all
        // share target 0.
        m_acceptAnyScreenTarget = true;
    }

    rive::gpu::RenderContext* renderContext() override
    {
        return RenderContextPtr;
    }

    rive::Renderer* beginScreen(uint64_t, bool, uint32_t) override
    {
        ScreenRenderer = BeginScreen();
        return ScreenRenderer.Get();
    }

private:
    rive::gpu::RenderContext* const RenderContextPtr;
    TFunctionRef<TUniquePtr<rive::Renderer>()> BeginScreen;
    TUniquePtr<rive::Renderer> ScreenRenderer;
};

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
        // Caps only, so recording never reaches for the device; the sink
        // hands the real ore context back at replay. Without one, 2D still
        // defers and only gpu canvas passes are lost.
        auto* Ore = RenderContext->ore();
        DeferredSession = MakeUnique<rive::cmd::DeferredSession>(
            Ore != nullptr ? rive::ore::ReplayCaps::from(*Ore)
                           : rive::ore::ReplayCaps{});
        // Scripts imported through the session talk to the device directly
        // while their canvas work records, so it has to be the real context.
        DeferredSession->bindRenderContext(RenderContext.get());
        InlineHost.bindSession(DeferredSession.Get());
        CommandServer = MakeUnique<rive::CommandServer>(CommandQueue,
                                                        DeferredSession.Get(),
                                                        GRiveOreShaderHandler);
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

    // Files outlive the server briefly, which the recorder registry no-ops by
    // design. Replay side resources are the real context's, so they go while
    // it is still alive.
    CommandServer.Reset();
    InlineHost.replayer().reset();
    InlineHost.bindSession(nullptr);
    DeferredSession.Reset();

    FCoreDelegates::OnBeginFrame.Remove(OnBeingFrameGameThreadHandle);
    FCoreDelegates::OnBeginFrame.Remove(OnEndFrameGameThreadHandle);
    FCoreDelegates::OnBeginFrameRT.Remove(OnBeingFrameRenderThreadHandle);
}

DECLARE_GPU_STAT_NAMED(BeingFrameRenderThread,
                       TEXT("FRiveRenderer::BeingFrameRenderThread"));
void FRiveRenderer::BeginFrameRenderThread()
{
    RHI_BREADCRUMB_EVENT_STAT(GRHICommandList.GetImmediateCommandList(),
                              BeingFrameRenderThread,
                              "BeingFrameRenderThread");

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

    // Deliver async work completions (script image decodes, etc.) before this
    // frame's script advance callbacks run. Upstream this pump lives in
    // Artboard::advance(), but StateMachineInstance::advanceAndApply which
    // is how artboards advance here, via CommandQueue::advanceStateMachine
    // calls advanceInternal() directly and skips it
    rive::rive_pollAsyncWork();

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

rive::Renderer* FRiveRenderer::BeginDeferredFrame()
{
    check(IsInRenderingThread());
    check(DeferredSession);

    // Every screen frame carries its own clear policy from the target it
    // opens, so the recording's is never read back.
    InlineHost.beginRecord(true, 0);
    return InlineHost.screenRenderer();
}

DECLARE_GPU_STAT_NAMED(ReplayDeferredFrame,
                       TEXT("FRiveRenderer::ReplayDeferredFrame"));
void FRiveRenderer::ReplayDeferredFrame(
    FRDGBuilder& GraphBuilder,
    TFunctionRef<TUniquePtr<rive::Renderer>()> BeginScreen,
    TFunctionRef<void()> Present)
{
    check(IsInRenderingThread());
    check(RenderContext);
    check(DeferredSession);

    SCOPED_GPU_STAT(GraphBuilder.RHICmdList, ReplayDeferredFrame);

    FRiveHostFrameSink Sink(RenderContext.get(),
                            BeginScreen,
                            InlineHost.doClear(),
                            InlineHost.clearColor(),
                            InlineHost.replayOre());

    // Canvas passes flush as replay reaches them, so they render into this
    // frame's builder rather than reaching for the one immediate mode Lua
    // canvases are given.
    RenderContextRHIImpl::FScopedExternalBuilder BuilderScope(GraphBuilder);

    InlineHost.replayInline(Sink, [&Present] { Present(); });
}

void FRiveRenderer::ReplayDeferredFrame(
    const TSharedPtr<FRiveRenderTarget>& RenderTarget)
{
    check(IsInRenderingThread());
    check(RenderTarget);

    FRDGBuilder GraphBuilder(GRHICommandList.GetImmediateCommandList());
    ReplayDeferredFrame(
        GraphBuilder,
        [this, &RenderTarget] {
            return RenderTarget->BeginRenderFrame(RenderContext.get());
        },
        [this, &RenderTarget, &GraphBuilder] {
            RenderContext->flush(
                {RenderTarget->GetRenderTarget().get(), &GraphBuilder});
        });
    GraphBuilder.Execute();
}

TSharedPtr<FRiveRenderTarget> FRiveRenderer::CreateRenderTarget(
    const FString& InRiveName,
    UTexture2DDynamic* InRenderTarget)
{
    check(IsInGameThread());

    const TSharedPtr<FRiveRenderTargetRHI> RiveRenderTarget =
        MakeShared<FRiveRenderTargetRHI>(this, InRiveName, InRenderTarget);

    return RiveRenderTarget;
}

TSharedPtr<FRiveRenderTarget> FRiveRenderer::CreateRenderTarget(
    const FString& InRiveName,
    UTextureRenderTarget2D* InRenderTarget)
{
    check(IsInGameThread());

    const TSharedPtr<FRiveRenderTargetRHI> RiveRenderTarget =
        MakeShared<FRiveRenderTargetRHI>(this, InRiveName, InRenderTarget);

    return RiveRenderTarget;
}

TSharedPtr<FRiveRenderTarget> FRiveRenderer::CreateRenderTarget(
    FRDGBuilder& GraphBuilder,
    const FString& InRiveName,
    FRDGTextureRef InRenderTarget)
{
    return MakeShared<FRiveRenderTargetRHI>(GraphBuilder,
                                            this,
                                            InRiveName,
                                            InRenderTarget);
    ;
}

TSharedPtr<FRiveRenderTarget> FRiveRenderer::CreateRenderTarget(
    const FString& InRiveName,
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

    RHI_BREADCRUMB_EVENT_STAT(RHICmdList,
                              CreateRenderContext,
                              "CreateRenderContext");

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
