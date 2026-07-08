// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveCommandBuilder.h"

#include "RiveRenderer.h"
#include "RiveRendererModule.h"
#include "RiveRenderTarget.h"
#include "RiveStats.h"
#include "RiveTypeConversions.h"
#include "Logs/RiveRendererLog.h"
#include "Platform/RenderContextRHIImpl.hpp"
#include "TextureResource.h"
#include "Async/Async.h"

#include <string>

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/artboard.hpp"
#include "rive/command_server.hpp"
#include "rive/logging_scripting_context.hpp"
#include "rive/renderer.hpp"
THIRD_PARTY_INCLUDES_END

// Routes a single line of Rive script output to LogRiveScripting. Invoked on
// the command server thread; UE_LOG is thread-safe. `Data` is valid only for
// the duration of the call.
static void RiveScriptingLogSink(rive::ScriptingLogLevel Level,
                                 const char* Data,
                                 size_t Length)
{
    const FUTF8ToTCHAR Conversion(Data, static_cast<int32>(Length));
    const FString Message(Conversion.Length(), Conversion.Get());
    switch (Level)
    {
        case rive::ScriptingLogLevel::error:
            UE_LOG(LogRiveScripting, Error, TEXT("%s"), *Message);
            break;
        case rive::ScriptingLogLevel::warn:
            UE_LOG(LogRiveScripting, Warning, TEXT("%s"), *Message);
            break;
        case rive::ScriptingLogLevel::info:
        default:
            UE_LOG(LogRiveScripting, Display, TEXT("%s"), *Message);
            break;
    }
}

rive::ScriptingContextFactory FRiveCommandBuilder::
    MakeScriptingLogContextFactory()
{
    return rive::makeLoggingScriptingContextFactory(&RiveScriptingLogSink);
}

FRiveCommandBuilder::FRiveCommandBuilder(
    rive::rcp<rive::CommandQueue> CommandQueue) :
    CommandQueue(CommandQueue)
{
    check(IsInGameThread());
}

rive::RenderImageHandle FRiveCommandBuilder::CreateRenderImage(
    UTexture* Value,
    uint64_t* outRequestId)
{
    // Make a strong reference to the texture so we can guarantee it won't get
    // GC'd.
    TStrongObjectPtr<UTexture> StrongValue(Value);
    auto RenderImageHandle = ExternalImages.Find(StrongValue);

    if (RenderImageHandle == nullptr)
    {
        auto RenderImage = RenderContextRHIImpl::MakeExternalRenderImage(Value);
        auto Handle = CommandQueue->addExternalImage(MoveTemp(RenderImage),
                                                     nullptr,
                                                     ++CurrentRequestId);
        ExternalImages.Add(StrongValue, Handle);
        return Handle;
    }

    return *RenderImageHandle;
}

uint64_t FRiveCommandBuilder::SetViewModelImage(
    rive::ViewModelInstanceHandle ViewModel,
    const FString& Name,
    UTexture* Value)
{
    FTCHARToUTF8 ConvertName(*Name);
    std::string ConvertedName(ConvertName.Get(), ConvertName.Length());

    uint64_t RequestId = 0;

    rive::RenderImageHandle RenderImageHandle =
        CreateRenderImage(Value, &RequestId);

    CommandQueue->setViewModelInstanceImage(ViewModel,
                                            MoveTemp(ConvertedName),
                                            RenderImageHandle,
                                            ++CurrentRequestId);

    return RequestId;
}

void FRiveCommandBuilder::RunOnce(ServerSideCallback Callback)
{
    Commands.Add(Callback);
}

void FRiveCommandBuilder::RunOnceImmediate(ServerSideCallback Callback)
{
    CommandQueue->runOnce(Callback);
}

void FRiveCommandBuilder::AdvanceStateMachine(rive::StateMachineHandle Handle,
                                              float AdvanceAmount)
{
    CommandQueue->advanceStateMachine(Handle,
                                      AdvanceAmount,
                                      ++CurrentRequestId);
}

void FRiveCommandBuilder::DrawArtboard(
    TSharedPtr<FRiveRenderTarget> RenderTarget,
    FDrawArtboardCommand DrawArtboardCommand)
{
    auto& RenderTargetDrawCommands =
        FindOrAddDrawCommands(std::move(RenderTarget));
    RenderTargetDrawCommands.DrawCommands.Add(
        {EDrawType::Artboard, nullptr, MoveTemp(DrawArtboardCommand)});
}

void FRiveCommandBuilder::Draw(TSharedPtr<FRiveRenderTarget> RenderTarget,
                               DirectDrawCallback Callback)
{
    auto& RenderTargetDrawCommands =
        FindOrAddDrawCommands(MoveTemp(RenderTarget));
    RenderTargetDrawCommands.DrawCommands.Add(
        {EDrawType::Direct, MoveTemp(Callback), {}});
}

static rive::AABB AABBFromAlignmentBox(const FBox2f& AlignmentBox)
{
    return rive::AABB(AlignmentBox.Min.X,
                      AlignmentBox.Min.Y,
                      AlignmentBox.Max.X,
                      AlignmentBox.Max.Y);
}

void FRiveCommandBuilder::DrawArtboard(const FDrawArtboardCommand& DrawCommand,
                                       rive::CommandServer* Server,
                                       rive::Renderer* Renderer)
{
    auto ArtboardInstance = Server->getArtboardInstance(DrawCommand.Handle);
    if (!ArtboardInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("DrawArtboard: No artboard instance"));
        return;
    }

    const auto Alignment = RiveAlignementToAlignment(DrawCommand.Alignment);
    const auto Fit = RiveFitTypeToFit(DrawCommand.FitType);
    const auto Frame = AABBFromAlignmentBox(DrawCommand.AlignmentBox);
    const auto Content = ArtboardInstance->bounds();

    Renderer->save();
    Renderer->align(Fit, Alignment, Frame, Content);
    ArtboardInstance->drawCanvases();
    ArtboardInstance->draw(Renderer);
    Renderer->restore();
    if (Fit == rive::Fit::layout)
    {
        ArtboardInstance->width(Frame.width());
        ArtboardInstance->height(Frame.height());
    }
    else
    {
        ArtboardInstance->resetSize();
    }
}

DECLARE_GPU_STAT_NAMED(RiveRenderTargetExecute,
                       TEXT("FRiveCommandBuilder::RiveRenderTargetExecute"));
void FRiveCommandBuilder::Execute()
{
    if (!Commands.IsEmpty())
    {
        CommandQueue->runOnce([Commands = MoveTemp(Commands)](
                                  rive::CommandServer* CommandServer) {
            UE_LOG(LogTemp,
                   Verbose,
                   TEXT("FRiveCommandBuilder::Execute RunOnce"));
            for (auto& Command : Commands)
            {
                Command(CommandServer);
            }
        });
    }

    for (auto& DrawCommand : DrawCommands)
    {
        auto RenderTarget = DrawCommand.Key;
        auto CommandSet = DrawCommand.Value;

        CommandQueue->draw(
            CommandSet.DrawKey,
            [RenderTarget, CommandSet](rive::DrawKey Key,
                                       rive::CommandServer* CommandServer) {
                auto& RHICmdList = GRHICommandList.GetImmediateCommandList();
                SCOPED_GPU_STAT(RHICmdList, RiveRenderTargetExecute);

                auto& RenderModulde = FRiveRendererModule::Get();
                check(RenderModulde.GetRenderer());
                auto RiveRenderContext =
                    RenderModulde.GetRenderer()->GetRenderContext();
                check(RiveRenderContext);

                auto Renderer =
                    RenderTarget->BeginRenderFrame(RiveRenderContext);
                auto Factory = CommandServer->factory();
                check(Factory);

                SCOPED_DRAW_EVENT(RHICmdList, RiveDrawArtboard);

                for (auto& DrawCommand : CommandSet.DrawCommands)
                {
                    switch (DrawCommand.DrawType)
                    {
                        case EDrawType::Artboard:
                            check(DrawCommand.ArtboardCommand.Handle !=
                                  RIVE_NULL_HANDLE);
                            DrawArtboard(DrawCommand.ArtboardCommand,
                                         CommandServer,
                                         Renderer.Get());
                            break;
                        case EDrawType::Direct:
                            check(DrawCommand.DrawCallback);
                            DrawCommand.DrawCallback(Key,
                                                     CommandServer,
                                                     Renderer.Get(),
                                                     Factory);
                            break;
                    }
                }

                RenderTarget->EndRenderFrame(RiveRenderContext);
            });
    }
}