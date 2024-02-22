// Copyright Rive, Inc. All rights reserved.

#include "Game/RiveActorComponent.h"

#include <Rive/RiveTexture.h>

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "RiveArtboard.h"
#include "URStateMachine.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveFile.h"

namespace UE::Rive::Core
{
    class FURStateMachine;
}

URiveActorComponent::URiveActorComponent(): Size(500, 500)
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;
}

void URiveActorComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeRenderTarget(Size.X, Size.Y);
}

void URiveActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsValidChecked(this))
    {
        return;
    }

    for (FRiveArtboardRenderData& ArtboardRenderData : RenderObjects) 
    {
        if (!ArtboardRenderData.bIsReady)
        {
            continue;
        }

        // RiveRenderTarget->Save();
        ArtboardRenderData.Artboard->Tick(DeltaTime);
        // RiveRenderTarget->Restore();
    }

    RiveRenderTarget->SubmitAndClear();
}

void URiveActorComponent::InitializeRenderTarget(int32 SizeX, int32 SizeY)
{
    UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    RenderTarget = NewObject<URiveTexture>();
    RenderTarget->ResizeRenderTargets(FIntPoint(SizeX, SizeY));

    // Initialize Rive Render Target Only after we resize the texture
    RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(*GetPathName(), RenderTarget);
    RiveRenderTarget->SetClearColor(FLinearColor::Transparent);
    RiveRenderTarget->Initialize();
}

void URiveActorComponent::ResizeRenderTarget(int32 InSizeX, int32 InSizeY)
{
    if (!RenderTarget)
    {
        return;
    }
	
    RenderTarget->ResizeRenderTargets(FIntPoint(InSizeX, InSizeY));
}

URiveArtboard* URiveActorComponent::InstantiateArtboard(URiveFile* InRiveFile, const FRiveArtboardRenderData& InArtboardRenderData)
{
    if (!InRiveFile)
    {
        UE_LOG(LogRive, Error, TEXT("Can't instantiate an artboard without a valid RiveFile."));
        return nullptr;
    }
	
    if (!UE::Rive::Renderer::IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer Module is either missing or not loaded properly."));
        return nullptr;
    }

    UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();

    if (!RiveRenderer)
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid renderer."));
        return nullptr;
    }

    if (!RiveRenderer->IsInitialized())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer is not initialized."));
        return nullptr;
    }

    FRiveArtboardRenderData Data = InArtboardRenderData;
    URiveArtboard* Artboard = NewObject<URiveArtboard>();
    Artboard->Initialize(InRiveFile->GetNativeFile(), RiveRenderTarget, Data.ArtboardName, Data.StateMachineName);
    Data.Artboard = Artboard;
    Data.bIsReady = true;
    RenderObjects.Add(Data);
    
    return Artboard;
}

// void URiveActorComponent::Tick_StateMachine(float DeltaTime, FRiveArtboardRenderData& InArtboardRenderData)
// {
//     // TODO: TickRiveReportedEvents.Empty();
//
//     UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
// 	
//     UE::Rive::Core::FURStateMachine* StateMachine = InArtboardRenderData.Artboard->GetStateMachine();
//     if (StateMachine && StateMachine->IsValid() && ensure(RiveRenderTarget))
//     {
//         FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
//         // if (!bIsReceivingInput)
//         // 		{
//         auto AdvanceStateMachine = [this, StateMachine, DeltaTime]()
//         {
//             // TODO: This
//             // if (StateMachine->HasAnyReportedEvents())
//             // {
//             // 	PopulateReportedEvents();
//             // }
// 				
// #if PLATFORM_ANDROID
//             UE_LOG(LogRive, Verbose, TEXT("[%s] StateMachine->Advance"), IsInRHIThread() ? TEXT("RHIThread") : (IsInRenderingThread() ? TEXT("RenderThread") : TEXT("OtherThread")));
// #endif
//             StateMachine->Advance(DeltaTime);
//         };
//         if (UE::Rive::Renderer::IRiveRendererModule::RunInGameThread())
//         {
//             AdvanceStateMachine();
//         }
//         else
//         {
//             ENQUEUE_RENDER_COMMAND(DrawArtboard)(
//                 [AdvanceStateMachine = MoveTemp(AdvanceStateMachine)](
//                 FRHICommandListImmediate& RHICmdList) mutable
//                 {
// #if PLATFORM_ANDROID
//                     RHICmdList.EnqueueLambda(TEXT("StateMachine->Advance"),
//                         [AdvanceStateMachine = MoveTemp(AdvanceStateMachine)](FRHICommandListImmediate& RHICmdList)
//                     {
// #endif
//                     AdvanceStateMachine();
// #if PLATFORM_ANDROID
//                     });
// #endif
//                 });
//         }
//     }
//     // }
// }
//
// void URiveActorComponent::Tick_Render(FRiveArtboardRenderData& InArtboardRenderData)
// {
//     RiveRenderTarget->Save();
//     RiveRenderTarget->Align(InArtboardRenderData.FitType, FRiveAlignment::GetAlignment(InArtboardRenderData.Alignment), InArtboardRenderData.Artboard->GetNativeArtboard());
//     RiveRenderTarget->Draw(InArtboardRenderData.Artboard->GetNativeArtboard());
//     RiveRenderTarget->Restore();
// }
