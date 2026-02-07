// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "Rive/RiveArtboard.h"

#include "InGamePerformanceTracker.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveStateMachine.h"
#include "Stats/RiveStats.h"
#include "Rive/RiveUtils.h"
#include "RiveRenderer.h"
#include "Rive/RiveViewModel.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/command_server.hpp"
#include "rive/renderer/rive_renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

class FRiveArtboardListener final : public rive::CommandQueue::ArtboardListener
{
public:
    FRiveArtboardListener(TWeakObjectPtr<URiveArtboard> ListeningArtboard) :
        ListeningArtboard(MoveTemp(ListeningArtboard))
    {}

    virtual void onArtboardError(const rive::ArtboardHandle Handle,
                                 uint64_t RequestId,
                                 std::string Error) override
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Artboard RequestId: %llu Error: %s"),
               RequestId,
               *FString(Error.c_str()));
        check(IsInGameThread());
        if (auto StrongArtboard = ListeningArtboard.Pin();
            StrongArtboard.IsValid())
        {
            StrongArtboard->ErrorReceived(RequestId);
        }
    }

    virtual void onDefaultViewModelInfoReceived(
        const rive::ArtboardHandle Handle,
        uint64_t RequestId,
        std::string ViewModelName,
        std::string InstanceName) override
    {
        check(IsInGameThread());
        if (auto StrongArtboard = ListeningArtboard.Pin();
            StrongArtboard.IsValid())
        {
#if WITH_EDITORONLY_DATA
            StrongArtboard->DefaultViewModelInfoReceived(
                std::move(ViewModelName),
                std::move(InstanceName));
#endif
        }
    }

    virtual void onArtboardDeleted(const rive::ArtboardHandle Handle,
                                   uint64_t RequestId) override
    {
        check(IsInGameThread());
        if (auto StrongArtboard = ListeningArtboard.Pin();
            StrongArtboard.IsValid())
        {
            UE_LOG(LogRive,
                   Display,
                   TEXT("Rive Artboard Named %s deleted"),
                   *StrongArtboard->GetName());
        }
        else
        {
            UE_LOG(LogRive,
                   Display,
                   TEXT("Rive Artboard Handle %p deleted"),
                   Handle);
        }

        delete this;
    }

    virtual void onStateMachinesListed(
        const rive::ArtboardHandle ArtboardHandle,
        uint64_t RequestId,
        std::vector<std::string> StateMachineNames) override
    {
        check(IsInGameThread());
        if (auto StrongArtboard = ListeningArtboard.Pin();
            StrongArtboard.IsValid())
        {
#if WITH_EDITORONLY_DATA
            StrongArtboard->StateMachinesListed(std::move(StateMachineNames));
#endif
        }
    }

private:
    TWeakObjectPtr<URiveArtboard> ListeningArtboard;
};

void URiveArtboard::BeginDestroy()
{
    if (!IsRunningCommandlet() && !HasAnyFlags(RF_ClassDefaultObject) &&
        NativeArtboardHandle != RIVE_NULL_HANDLE)
    {
        auto Renderer = IRiveRendererModule::Get().GetRenderer();
        check(Renderer);
        auto& CommandBuilder = Renderer->GetCommandBuilder();

        if (StateMachine.IsValid())
            StateMachine->Destroy(CommandBuilder);

        CommandBuilder.DestroyArtboard(NativeArtboardHandle);
    }

    Super::BeginDestroy();
}

void URiveArtboard::DrawToRenderTarget(
    FRiveCommandBuilder& CommandBuilder,
    TSharedPtr<FRiveRenderTarget> RenderTarget)
{
    CommandBuilder.DrawArtboard(RenderTarget, {NativeArtboardHandle});
}

void URiveArtboard::Initialize(URiveFile* InRiveFile,
                               const FArtboardDefinition& InDefinition,
                               bool InAutoBindViewModel,
                               FRiveCommandBuilder& InCommandBuilder)
{
    Initialize(InRiveFile,
               InDefinition,
               "",
               InAutoBindViewModel,
               InCommandBuilder);
}

void URiveArtboard::Initialize(URiveFile* InRiveFile,
                               const FArtboardDefinition& InDefinition,
                               const FString& InStateMachineName,
                               bool InAutoBindViewModel,
                               FRiveCommandBuilder& InCommandBuilder)
{
    if (!IsValid(InRiveFile))
    {
        UE_LOG(LogRive, Error, TEXT("Invalid RiveFile passed to Initialize."));
        return;
    }

    RiveFile = InRiveFile;
    ArtboardDefinition = InDefinition;

    if (ArtboardDefinition.Name.IsEmpty())
    {
        NativeArtboardHandle = InCommandBuilder.CreateDefaultArtboard(
            InRiveFile->GetNativeFileHandle(),
            new FRiveArtboardListener(this));
    }
    else
    {
        NativeArtboardHandle =
            InCommandBuilder.CreateArtboard(InRiveFile->GetNativeFileHandle(),
                                            InDefinition.Name,
                                            new FRiveArtboardListener(this));
    }

    SetupStateMachine(InCommandBuilder,
                      InStateMachineName,
                      InAutoBindViewModel);
}
#if WITH_EDITORONLY_DATA
void URiveArtboard::StateMachinesListed(
    std::vector<std::string> InStateMachineNames)
{
    StateMachineNames.Empty();

    for (const auto& StateMachineName : InStateMachineNames)
    {
        FUTF8ToTCHAR Conversion(StateMachineName.c_str());
        StateMachineNames.Add(Conversion.Get());
    }
    bHasStateMachineNames = true;
    BroadcastHasDataIfReady();
}

void URiveArtboard::DefaultViewModelInfoReceived(std::string ViewModel,
                                                 std::string ViewModelInstance)
{
    DefaultViewModelName = FString(ViewModel.c_str());
    DefaultViewModelInstanceName = FString(ViewModelInstance.c_str());
    bHasDefaultViewModel = true;
    bHasDefaultViewModelInfo = true;
    BroadcastHasDataIfReady();
}
#endif

void URiveArtboard::SetupStateMachine(const FString& StateMachineName,
                                      bool InAutoBindViewModel)
{
    auto Renderer = IRiveRendererModule::Get().GetRenderer();
    check(Renderer);
    auto& CommandBuilder = Renderer->GetCommandBuilder();
    SetupStateMachine(CommandBuilder, StateMachineName, InAutoBindViewModel);
}

void URiveArtboard::SetupStateMachine(FRiveCommandBuilder& InCommandBuilder,
                                      const FString& InStateMachineName,
                                      bool InAutoBindViewModel)
{
    StateMachine = MakeShared<FRiveStateMachine>();
    StateMachineCreateRequestId = StateMachine->Initialize(InCommandBuilder,
                                                           NativeArtboardHandle,
                                                           InStateMachineName);
    StateMachine->Advance(InCommandBuilder, 0); // Just to setup everything.

    // Legacy code that make artboards draw without state machines.
    InCommandBuilder.RunOnce(
        [ArtboardHandle = NativeArtboardHandle,
         StateMachineHandle = StateMachine->GetNativeStateMachineHandle()](
            rive::CommandServer* Server) {
            // No need if State Machine exists.
            if (auto StateMachinePtr =
                    Server->getStateMachineInstance(StateMachineHandle))
            {
                return;
            }

            if (auto ArtboardPtr = Server->getArtboardInstance(ArtboardHandle))
            {
                ArtboardPtr->advance(1.f);
            }
        });

    if (InAutoBindViewModel)
    {
        FString ViewModelName = ArtboardDefinition.DefaultViewModel;
        if (!ArtboardDefinition.DefaultViewModelInstance.IsEmpty())
        {
            ViewModelName +=
                TEXT("_") + ArtboardDefinition.DefaultViewModelInstance;
        }
        ViewModelName = RiveUtils::SanitizeObjectName(ViewModelName);
        auto ViewModelDef = RiveFile->GetViewModelDefinition(
            ArtboardDefinition.DefaultViewModel);
        if (!ViewModelDef)
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("Failed to autobind view mode %s because it is not "
                        "found in file %s"),
                   *ArtboardDefinition.Name,
                   *RiveFile->GetName());
            return;
        }
        auto ViewModel =
            NewObject<URiveViewModel>(RiveFile->GetOuter(), *ViewModelName);
        ViewModel->Initialize(InCommandBuilder,
                              RiveFile.Get(),
                              *ViewModelDef,
                              GetDefaultViewModelInstance());
        StateMachine->BindViewModel(ViewModel);
        BoundViewModel = ViewModel;
    }
}

void URiveArtboard::ErrorReceived(uint64_t RequestId)
{
    if (StateMachineCreateRequestId == RequestId && StateMachine.IsValid())
    {
        StateMachine->SetValid(false);
        auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
        check(RiveRenderer);
        auto& CommandBuilder = RiveRenderer->GetCommandBuilder();
        TWeakObjectPtr<URiveArtboard> WeakThis(this);
        CommandBuilder.RunOnce([WeakThis](rive::CommandServer* Server) {
            if (auto Artboard = WeakThis.Pin(); Artboard.IsValid())
            {
                if (auto instance = Server->getArtboardInstance(
                        Artboard->GetNativeArtboardHandle()))
                {
                    if (instance->animationCount() > 0)
                    {
                        Artboard->LinearAnimation = instance->animationAt(0);
                    }
                }
            }
        });
    }
#if WITH_EDITORONLY_DATA
    // There is not a default view model for this artboard.
    if (RequestId == GetDefaultViewModelRequestId)
    {
        bHasDefaultViewModel = false;
        bHasDefaultViewModelInfo = true;
        DefaultViewModelName = "";
        DefaultViewModelInstanceName = "";
        BroadcastHasDataIfReady();
    }
#endif
}

void URiveArtboard::SetStateMachine(const FString& StateMachineName,
                                    bool InAutoBindViewModel)
{
    SetupStateMachine(StateMachineName, InAutoBindViewModel);
}

void URiveArtboard::SetViewModel(URiveViewModel* RiveViewModelInstance)
{
    if (!ensure(IsValid(RiveViewModelInstance)))
    {
        UE_LOG(
            LogRive,
            Error,
            TEXT("URiveArtboard::SetViewModel Invalid RiveViewModelInstance"));
        return;
    }

    RiveViewModelInstance->SetOwningArtboard(this);

    if (StateMachine && StateMachine->IsValid())
    {
        StateMachine->BindViewModel(RiveViewModelInstance);
        BoundViewModel = RiveViewModelInstance;
    }
    else
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("URiveArtboard::SetViewModel No State Machine to set view "
                    "model on."));
    }
}

rive::StateMachineHandle URiveArtboard::GetStateMachineHandle() const
{
    if (StateMachine.IsValid())
        return StateMachine->GetNativeStateMachineHandle();
    return RIVE_NULL_HANDLE;
}

void URiveArtboard::SetAudioEngine(URiveAudioEngine* AudioEngine)
{
    auto& CommandBuilder =
        IRiveRendererModule::Get().GetRenderer()->GetCommandBuilder();

    CommandBuilder.RunOnce(
        [AudioEngine, NativeArtboardHandle = NativeArtboardHandle](
            rive::CommandServer* server) {
            auto NativeArtboardPtr =
                server->getArtboardInstance(NativeArtboardHandle);

            if (NativeArtboardPtr == nullptr)
            {
                UE_LOG(LogRive,
                       Error,
                       TEXT("URiveArtboard::SetAudioEngine native artboard "
                            "instance is null"));
                return;
            }

            if (AudioEngine == nullptr)
            {
                rive::rcp<rive::AudioEngine> NativeEngine =
                    NativeArtboardPtr->audioEngine();

                if (NativeEngine != nullptr)
                {
                    NativeEngine->unref();
                }
                NativeArtboardPtr->audioEngine(nullptr);
                return;
            }

            NativeArtboardPtr->audioEngine(AudioEngine->GetNativeAudioEngine());
        });
}

#if WITH_EDITORONLY_DATA

void URiveArtboard::InitializeForDataImport(
    URiveFile* InRiveFile,
    const FString& Name,
    FRiveCommandBuilder& InCommandBuilder)
{
    ArtboardDefinition.Name = Name;

    if (Name.IsEmpty())
    {
        NativeArtboardHandle = InCommandBuilder.CreateDefaultArtboard(
            InRiveFile->GetNativeFileHandle(),
            new FRiveArtboardListener(this));
    }
    else
    {
        NativeArtboardHandle =
            InCommandBuilder.CreateArtboard(InRiveFile->GetNativeFileHandle(),
                                            Name,
                                            new FRiveArtboardListener(this));
    }

    InCommandBuilder.RequestStateMachineNames(NativeArtboardHandle);
    GetDefaultViewModelRequestId = InCommandBuilder.RequestDefaultViewModelInfo(
        NativeArtboardHandle,
        InRiveFile->GetNativeFileHandle());
    // We don't want to get GC'd before this callback. So make a strong
    // reference first.
    TStrongObjectPtr<URiveArtboard> StrongThis(this);
    InCommandBuilder.RunOnce([StrongThis](rive::CommandServer* CommandServer) {
        auto NativeArtboardPtr = CommandServer->getArtboardInstance(
            StrongThis->NativeArtboardHandle);
        if (!NativeArtboardPtr)
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("No artboard found when trying to get default size."));
            return;
        }
        // This is in the render thread, so dispatch back to game thread to
        // update.
        float Width = NativeArtboardPtr->originalWidth();
        float Height = NativeArtboardPtr->originalHeight();

        // Get the artboard size.
        AsyncTask(ENamedThreads::GameThread, [StrongThis, Width, Height]() {
            StrongThis->ArtboardDefaultSize = FVector2D(Width, Height);
            StrongThis->bHasDefaultArtboardSize = true;
            StrongThis->BroadcastHasDataIfReady();
        });
    });
}
#endif
TStatId URiveArtboard::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FArtboardTick, STATGROUP_Rive);
}

void URiveArtboard::Tick(float InDeltaSeconds)
{
    if (NativeArtboardHandle == RIVE_NULL_HANDLE)
    {
        return;
    }

    if (StateMachine.IsValid() && StateMachine->IsValid() &&
        !StateMachine->IsStateMachineSettled())
    {
        auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
        check(RiveRenderer);
        auto& CommandBuilder = RiveRenderer->GetCommandBuilder();
        StateMachine->Advance(CommandBuilder, InDeltaSeconds);
    }
    else
    {
        auto RiveRenderer = IRiveRendererModule::Get().GetRenderer();
        check(RiveRenderer);
        auto& CommandBuilder = RiveRenderer->GetCommandBuilder();
        TWeakObjectPtr<URiveArtboard> WeakThis(this);
        CommandBuilder.RunOnce([WeakThis,
                                InDeltaSeconds](rive::CommandServer* Server) {
            auto StrongThis = WeakThis.Pin();
            if (!StrongThis)
                return;

            if (StrongThis->LinearAnimation)
            {
                StrongThis->LinearAnimation->advanceAndApply(InDeltaSeconds);
            }
            else
            {
                if (auto artboardInstance = Server->getArtboardInstance(
                        StrongThis->NativeArtboardHandle))
                {
                    artboardInstance->advance(InDeltaSeconds);
                }
            }
        });
    }
}

void URiveArtboard::SetNativeArtboardSizeWithScale(float Width,
                                                   float Height,
                                                   float Scale)
{
    auto& Builder = IRiveRendererModule::Get().GetCommandBuilder();
    Builder.SetArtboardSize(NativeArtboardHandle, Width, Height, Scale);
    if (StateMachine.IsValid())
    {
        StateMachine->Advance(Builder, 0);
    }
}

void URiveArtboard::ResetNativeArtboardSize()
{
    auto& Builder = IRiveRendererModule::Get().GetCommandBuilder();
    Builder.ResetArtboardSize(NativeArtboardHandle);
    if (StateMachine.IsValid())
    {
        StateMachine->Advance(Builder, 0);
    }
}

void URiveArtboard::UnsettleStateMachine()
{
    if (StateMachine.IsValid())
    {
        StateMachine->SetStateMachineSettled(false);
    }
}
