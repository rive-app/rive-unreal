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

uint64_t FRiveCommandBuilder::SetViewModelImage(
    rive::ViewModelInstanceHandle ViewModel,
    const FString& Name,
    UTexture* Value)
{
    FTCHARToUTF8 ConvertName(*Name);
    std::string ConvertedName(ConvertName.Get(), ConvertName.Length());

    // This mess is to support using UTexture2D directly as a value for a
    // property instead of forcing people to use the command queue to create a
    // render image first.

    // Make a strong reference to the texture so we can guarantee it won't get
    // GC'd.
    TStrongObjectPtr<UTexture> StrongValue(Value);
    auto RenderImageHandle = ExternalImages.Find(StrongValue);
    if (RenderImageHandle == nullptr)
    {
        // If we don't already have this texture we first dispatch to the render
        // thread to create a render image.
        RunOnce([this, StrongValue, ViewModel, Name = MoveTemp(ConvertedName)](
                    rive::CommandServer* CommandServer) {
            // Everything in here is on the render thread.
            check(IsInRenderingThread());
            auto RHITexture = StrongValue->GetResource()->GetTexture2DRHI();
            auto RenderImage =
                RenderContextRHIImpl::MakeExternalRenderImage(RHITexture);

            // Now we dispatch back to the game thread because CommandQueue
            // should only ever be used there.
            AsyncTask(
                ENamedThreads::GameThread,
                [this, RenderImage, ViewModel, StrongValue, Name]() {
                    auto Handle = CommandQueue->addExternalImage(RenderImage);
                    ExternalImages.Add(StrongValue, Handle);
                    CommandQueue->setViewModelInstanceImage(ViewModel,
                                                            Name,
                                                            Handle,
                                                            ++CurrentRequestId);
                });
        });

        return -1; // -1 Here means we have to actually do the set async
    }

    CommandQueue->setViewModelInstanceImage(ViewModel,
                                            ConvertName.Get(),
                                            *RenderImageHandle,
                                            ++CurrentRequestId);
    return CurrentRequestId;
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
    CommandQueue->advanceStateMachine(Handle, AdvanceAmount);
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
    CommandQueue->runOnce([Commands =
                               Commands](rive::CommandServer* CommandServer) {
        UE_LOG(LogTemp, Verbose, TEXT("FRiveCommandBuilder::Execute RunOnce"));
        for (auto& Command : Commands)
        {
            Command(CommandServer);
        }
    });

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