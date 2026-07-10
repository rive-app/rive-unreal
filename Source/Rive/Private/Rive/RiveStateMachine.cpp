// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "Rive/RiveStateMachine.h"
#include "Rive/RiveViewModel.h"

#include "IRiveRendererModule.h"
#include "RiveCommandBuilder.h"
#include "RiveRenderer.h"
#include "RiveTypeConversions.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Logs/RiveLog.h"
#include "rive/command_queue.hpp"
#include "rive/command_server.hpp"
#include "Rive/RiveDescriptor.h"
#include "Stats/RiveStats.h"
#include "Rive/RiveUtils.h"
#include "RenderingThread.h"

#include <atomic>

#if WITH_RIVE

class FRiveStateMachineListener final
    : public rive::CommandQueue::StateMachineListener
{
public:
    FRiveStateMachineListener(
        TWeakPtr<FRiveStateMachine> ListeningStateMachine) :
        ListeningStateMachine(MoveTemp(ListeningStateMachine))
    {}

    virtual void onStateMachineError(const rive::StateMachineHandle,
                                     uint64_t requestId,
                                     std::string error) override
    {
        check(IsInGameThread());
        if (auto StrongStateMachine = ListeningStateMachine.Pin();
            StrongStateMachine.IsValid())
        {
            StrongStateMachine->OnStateMachineError(requestId,
                                                    std::move(error));
        }
    }

    virtual void onStateMachineDeleted(const rive::StateMachineHandle Handle,
                                       uint64_t requestId) override
    {
        check(IsInGameThread());
        if (auto StrongStateMachine = ListeningStateMachine.Pin();
            StrongStateMachine.IsValid())
        {
            UE_LOG(LogRive,
                   Display,
                   TEXT("Rive StateMachine Named %s deleted"),
                   *StrongStateMachine->GetStateMachineName());
        }
        else
        {
            UE_LOG(LogRive,
                   Display,
                   TEXT("Rive StateMachine Handle %p deleted"),
                   Handle);
        }

        delete this;
    }
    // requestId in this case is the specific request that caused the
    // statemachine to settle
    virtual void onStateMachineSettled(const rive::StateMachineHandle,
                                       uint64_t requestId) override
    {
        check(IsInGameThread());
        if (auto StrongStateMachine = ListeningStateMachine.Pin();
            StrongStateMachine.IsValid())
        {
            StrongStateMachine->SetStateMachineSettled(true);
        }
    }

private:
    TWeakPtr<FRiveStateMachine> ListeningStateMachine;
};

rive::EventReport FRiveStateMachine::NullEvent =
    rive::EventReport(nullptr, 0.f);

void FRiveStateMachine::Destroy(FRiveCommandBuilder& CommandBuilder)
{
    CommandBuilder.DestroyStateMachine(NativeStateMachineHandle);
}

uint64_t FRiveStateMachine::Initialize(
    FRiveCommandBuilder& CommandBuilder,
    rive::ArtboardHandle InOwningArtboardHandle,
    const FString& InStateMachineName)
{
    uint64_t CreateRequestId = 0;
    bStateMachineSettled = false;
    if (InStateMachineName.IsEmpty())
    {
        NativeStateMachineHandle = CommandBuilder.CreateDefaultStateMachine(
            InOwningArtboardHandle,
            new FRiveStateMachineListener(AsWeak()),
            &CreateRequestId);
    }
    else
    {
        NativeStateMachineHandle = CommandBuilder.CreateStateMachine(
            InOwningArtboardHandle,
            InStateMachineName,
            new FRiveStateMachineListener(AsWeak()),
            &CreateRequestId);
        StateMachineName = InStateMachineName;
    }

    return CreateRequestId;
}

void FRiveStateMachine::Advance(FRiveCommandBuilder& CommandBuilder,
                                float InSeconds)
{
    SCOPED_NAMED_EVENT_TEXT(TEXT("FRiveStateMachine::Advance"), FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FRiveStateMachine::Advance"),
                                STAT_STATEMACHINE_ADVANCE,
                                STATGROUP_Rive);
    CommandBuilder.AdvanceStateMachine(NativeStateMachineHandle, InSeconds);
}

uint32 FRiveStateMachine::GetInputCount() const { return 0; }

bool FRiveStateMachine::PointerDown(const FRiveDescriptor& InDescriptor,
                                    const FVector2D& NormalLocationOnSurface)
{
    bStateMachineSettled = false;
    auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
    CommandBuilder.StateMachineMouseDown(
        NativeStateMachineHandle,
        {.fit = RiveFitTypeToFit(InDescriptor.FitType),
         .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
         // Normalized Bounds 0-1
         .screenBounds = {1, 1},
         .position = {static_cast<float>(NormalLocationOnSurface.X),
                      static_cast<float>(NormalLocationOnSurface.Y)},
         .scaleFactor = InDescriptor.ScaleFactor});
    return false;
}

bool FRiveStateMachine::PointerMove(const FRiveDescriptor& InDescriptor,
                                    const FVector2D& NormalLocationOnSurface)
{
    bStateMachineSettled = false;
    auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
    CommandBuilder.StateMachineMouseMove(
        NativeStateMachineHandle,
        {.fit = RiveFitTypeToFit(InDescriptor.FitType),
         .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
         // Normalized Bounds 0-1
         .screenBounds = {1, 1},
         .position = {static_cast<float>(NormalLocationOnSurface.X),
                      static_cast<float>(NormalLocationOnSurface.Y)},
         .scaleFactor = InDescriptor.ScaleFactor});
    return false;
}

bool FRiveStateMachine::PointerUp(const FRiveDescriptor& InDescriptor,
                                  const FVector2D& NormalLocationOnSurface)
{
    bStateMachineSettled = false;
    auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
    CommandBuilder.StateMachineMouseUp(
        NativeStateMachineHandle,
        {.fit = RiveFitTypeToFit(InDescriptor.FitType),
         .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
         // Normalized Bounds 0-1
         .screenBounds = {1, 1},
         .position = {static_cast<float>(NormalLocationOnSurface.X),
                      static_cast<float>(NormalLocationOnSurface.Y)},
         .scaleFactor = InDescriptor.ScaleFactor});
    return false;
}

bool FRiveStateMachine::PointerExit(const FRiveDescriptor& InDescriptor,
                                    const FVector2D& NormalLocationOnSurface)
{
    bStateMachineSettled = false;
    auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
    CommandBuilder.StateMachineMouseOut(
        NativeStateMachineHandle,
        {.fit = RiveFitTypeToFit(InDescriptor.FitType),
         .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
         // Normalized Bounds 0-1
         .screenBounds = {1, 1},
         .position = {static_cast<float>(NormalLocationOnSurface.X),
                      static_cast<float>(NormalLocationOnSurface.Y)},
         .scaleFactor = InDescriptor.ScaleFactor});
    return false;
}

// Runs a pointer event on the command server thread and blocks for its
// HitResult. Queued via runOnce so it stays FIFO with pending commands
// (moves, advances). The render command forces an early processCommands():
// the per-frame drain in BeginFrameRT can't run while the game thread blocks
// here. Timeout covers a suspended render thread (resize, teardown).
static rive::HitResult SendPointerEventAndWait(
    rive::StateMachineHandle Handle,
    const rive::CommandQueue::PointerEvent& Event,
    rive::HitResult (rive::CommandServer::*Send)(
        rive::StateMachineHandle,
        const rive::CommandQueue::PointerEvent&))
{
    FRiveRenderer* Renderer = IRiveRendererModule::Get().GetRenderer();
    check(Renderer);

    if (IsInRenderingThread())
    {
        return (Renderer->GetCommandServer()->*Send)(Handle, Event);
    }

    struct FPending
    {
        FEventRef Done;
        std::atomic<rive::HitResult> Result{rive::HitResult::none};
    };
    // Shared so a timed-out wait can return while the callback still runs.
    auto Pending = MakeShared<FPending>();

    Renderer->GetCommandBuilder().RunOnceImmediate(
        [Pending, Handle, Event, Send](rive::CommandServer* Server) {
            Pending->Result = (Server->*Send)(Handle, Event);
            Pending->Done->Trigger();
        });

    ENQUEUE_RENDER_COMMAND(RivePointerEventFlush)
    ([Renderer](FRHICommandListImmediate&) {
        if (auto* Server = Renderer->GetCommandServer())
        {
            Server->processCommands();
        }
    });

    constexpr uint32 TimeoutMs = 100;
    if (!Pending->Done->Wait(TimeoutMs))
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("Pointer event timed out waiting on command server; "
                    "treating as a miss"));
        return rive::HitResult::none;
    }
    return Pending->Result;
}

bool FRiveStateMachine::PointerDown(const FGeometry& InGeometry,
                                    const FRiveDescriptor& InDescriptor,
                                    const FPointerEvent& InMouseEvent,
                                    float DPI)
{
    bStateMachineSettled = false;
    float ScaleFactor = 1.0f;

    FVector2D Position = USlateBlueprintLibrary::AbsoluteToLocal(
        InGeometry,
        InMouseEvent.GetScreenSpacePosition());
    FVector2D ScreenBounds = InGeometry.GetLocalSize();

    if (InDescriptor.FitType == ERiveFitType::Layout)
    {
        ScaleFactor = InDescriptor.ScaleFactor;
        if (InDescriptor.bScaleLayoutByDPI)
            ScaleFactor *= DPI;
        Position *= DPI;
        ScreenBounds *= DPI;
    }

    return SendPointerEventAndWait(
               NativeStateMachineHandle,
               {.fit = RiveFitTypeToFit(InDescriptor.FitType),
                .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
                .screenBounds = {static_cast<float>(ScreenBounds.X),
                                 static_cast<float>(ScreenBounds.Y)},
                .position = {static_cast<float>(Position.X),
                             static_cast<float>(Position.Y)},
                .scaleFactor = ScaleFactor},
               &rive::CommandServer::pointerDownSynchronized) !=
           rive::HitResult::none;
}

bool FRiveStateMachine::PointerMove(const FGeometry& InGeometry,
                                    const FRiveDescriptor& InDescriptor,
                                    const FPointerEvent& InMouseEvent,
                                    float DPI)
{
    // Ignore when the mouse doesn't move.
    if (InMouseEvent.GetCursorDelta().GetAbsMax() <= 0)
    {
        return false;
    }
    bStateMachineSettled = false;

    FVector2D Position = USlateBlueprintLibrary::AbsoluteToLocal(
        InGeometry,
        InMouseEvent.GetScreenSpacePosition());
    FVector2D ScreenBounds = InGeometry.GetLocalSize();

    float ScaleFactor = 1.0f;
    if (InDescriptor.FitType == ERiveFitType::Layout)
    {
        ScaleFactor = InDescriptor.ScaleFactor;
        if (InDescriptor.bScaleLayoutByDPI)
            ScaleFactor *= DPI;
        Position *= DPI;
        ScreenBounds *= DPI;
    }

    FRiveRenderer* Renderer = IRiveRendererModule::Get().GetRenderer();
    check(Renderer);
    auto& CommandBuilder = Renderer->GetCommandBuilder();
    CommandBuilder.StateMachineMouseMove(
        NativeStateMachineHandle,
        {.fit = RiveFitTypeToFit(InDescriptor.FitType),
         .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
         .screenBounds = {static_cast<float>(ScreenBounds.X),
                          static_cast<float>(ScreenBounds.Y)},
         .position = {static_cast<float>(Position.X),
                      static_cast<float>(Position.Y)},
         .scaleFactor = ScaleFactor});

    return false;
}

bool FRiveStateMachine::PointerUp(const FGeometry& InGeometry,
                                  const FRiveDescriptor& InDescriptor,
                                  const FPointerEvent& InMouseEvent,
                                  float DPI)
{
    bStateMachineSettled = false;
    FVector2D Position = USlateBlueprintLibrary::AbsoluteToLocal(
        InGeometry,
        InMouseEvent.GetScreenSpacePosition());
    FVector2D ScreenBounds = InGeometry.GetLocalSize();

    float ScaleFactor = 1.0f;
    if (InDescriptor.FitType == ERiveFitType::Layout)
    {
        ScaleFactor = InDescriptor.ScaleFactor;
        if (InDescriptor.bScaleLayoutByDPI)
            ScaleFactor *= DPI;
        Position *= DPI;
        ScreenBounds *= DPI;
    }

    return SendPointerEventAndWait(
               NativeStateMachineHandle,
               {.fit = RiveFitTypeToFit(InDescriptor.FitType),
                .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
                .screenBounds = {static_cast<float>(ScreenBounds.X),
                                 static_cast<float>(ScreenBounds.Y)},
                .position = {static_cast<float>(Position.X),
                             static_cast<float>(Position.Y)},
                .scaleFactor = ScaleFactor},
               &rive::CommandServer::pointerUpSynchronized) !=
           rive::HitResult::none;
}

bool FRiveStateMachine::PointerExit(const FGeometry& InGeometry,
                                    const FRiveDescriptor& InDescriptor,
                                    const FPointerEvent& InMouseEvent,
                                    float DPI)
{
    bStateMachineSettled = false;
    float ScaleFactor = 1.0f;
    if (InDescriptor.FitType == ERiveFitType::Layout)
    {
        ScaleFactor = InDescriptor.ScaleFactor;
        if (InDescriptor.bScaleLayoutByDPI)
            ScaleFactor *= DPI;
    }

    FVector2D Position = USlateBlueprintLibrary::AbsoluteToLocal(
        InGeometry,
        InMouseEvent.GetScreenSpacePosition());
    FVector2D ScreenBounds = InGeometry.GetLocalSize();

    FRiveRenderer* Renderer = IRiveRendererModule::Get().GetRenderer();
    check(Renderer);
    auto& CommandBuilder = Renderer->GetCommandBuilder();
    CommandBuilder.StateMachineMouseOut(
        NativeStateMachineHandle,
        {.fit = RiveFitTypeToFit(InDescriptor.FitType),
         .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
         .screenBounds = {static_cast<float>(ScreenBounds.X),
                          static_cast<float>(ScreenBounds.Y)},
         .position = {static_cast<float>(Position.X),
                      static_cast<float>(Position.Y)},
         .scaleFactor = ScaleFactor});

    return false;
}

void FRiveStateMachine::BindViewModel(TObjectPtr<URiveViewModel> ViewModel)
{
    bStateMachineSettled = false;
    if (!::IsValid(ViewModel))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("FRiveStateMachine::BindViewModel ViewModel was invalid"));
        return;
    }
    FRiveRenderer* Renderer = IRiveRendererModule::Get().GetRenderer();
    check(Renderer);
    auto& CommandBuilder = Renderer->GetCommandBuilder();
    CommandBuilder.StateMachineBindViewModel(NativeStateMachineHandle,
                                             ViewModel->GetNativeHandle());
}

void FRiveStateMachine::SetStateMachineSettled(bool inStateMachineSettled)
{
    UE_LOG(LogRive,
           VeryVerbose,
           TEXT("Rive StateMachine %s SetSettled %s"),
           *StateMachineName,
           inStateMachineSettled ? TEXT("True") : TEXT("False"));
    bStateMachineSettled = inStateMachineSettled;
}

void FRiveStateMachine::OnStateMachineError(uint64_t requestId,
                                            std::string error)
{}

#endif // WITH_RIVE
