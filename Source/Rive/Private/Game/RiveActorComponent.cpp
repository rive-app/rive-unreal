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
