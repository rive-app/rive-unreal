// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RiveCommandBuilder.h"

#include "RiveRenderer.h"
#include "RiveRendererModule.h"
#include "RiveRenderTarget.h"
#include "RiveStats.h"
#include "RiveTypeConversions.h"
#include "Platform/RenderContextRHIImpl.hpp"
#include "TextureResource.h"
#include "Async/Async.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/artboard.hpp"
#include "rive/command_server.hpp"
#include "rive/renderer.hpp"
THIRD_PARTY_INCLUDES_END

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

    Renderer->save();
    Renderer->align(Fit, Alignment, Frame, ArtboardInstance->bounds());
    ArtboardInstance->draw(Renderer);
    Renderer->restore();
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