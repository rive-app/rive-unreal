// Copyright Rive, Inc. All rights reserved.

#include "Game/RiveActorComponent.h"

#include <Rive/RiveTexture.h>

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "RiveArtboard.h"
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
    InitializeRenderTarget(Size.X, Size.Y);
    Super::BeginPlay();
}

void URiveActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsValidChecked(this))
    {
        return;
    }

    if (RiveRenderTarget)
    {
        for (URiveArtboard* Artboard : RenderObjects)
        {
            RiveRenderTarget->Save();
            Artboard->Tick(DeltaTime);
            RiveRenderTarget->Restore();
        }


        RiveRenderTarget->SubmitAndClear();
    }
}

void URiveActorComponent::InitializeRenderTarget(int32 SizeX, int32 SizeY)
{
    UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive, Error, TEXT("RiveRenderer is null, unable to initialize the RenderTarget for Rive file '%s'"), *GetFullNameSafe(this));
        return;
    }
    
    RiveRenderer->CallOrRegister_OnInitialized(UE::Rive::Renderer::IRiveRenderer::FOnRendererInitialized::FDelegate::CreateLambda(
    [this, SizeX, SizeY](UE::Rive::Renderer::IRiveRenderer* InRiveRenderer)
    {
        RenderTarget = NewObject<URiveTexture>();
        // Initialize Rive Render Target Only after we resize the texture
        RiveRenderTarget = InRiveRenderer->CreateTextureTarget_GameThread(GetFName(), RenderTarget);
        RiveRenderTarget->SetClearColor(FLinearColor::Transparent);
        RenderTarget->ResizeRenderTargets(FIntPoint(SizeX, SizeY));
        RiveRenderTarget->Initialize();

        RenderTarget->OnResourceInitializedOnRenderThread.AddUObject(this, &URiveActorComponent::OnResourceInitialized_RenderThread);
        OnRiveReady.Broadcast();
    }));
}

void URiveActorComponent::ResizeRenderTarget(int32 InSizeX, int32 InSizeY)
{
    if (!RenderTarget)
    {
        return;
    }
	
    RenderTarget->ResizeRenderTargets(FIntPoint(InSizeX, InSizeY));
}

URiveArtboard* URiveActorComponent::InstantiateArtboard(URiveFile* InRiveFile, const FString& InArtboardName, const FString& InStateMachineName)
{
    if (!IsValid(InRiveFile))
    {
        UE_LOG(LogRive, Error, TEXT("Can't instantiate an artboard without a valid RiveFile."));
        return nullptr;
    }
    if (!InRiveFile->IsInitialized())
    {
        UE_LOG(LogRive, Error, TEXT("Can't instantiate an artboard a RiveFile that is not initialized!"));
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
        UE_LOG(LogRive, Error, TEXT("Failed to instantiate the Artboard of Rive file '%s' as we do not have a valid renderer."), *GetFullNameSafe(InRiveFile));
        return nullptr;
    }

    if (!RiveRenderer->IsInitialized())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer is not initialized."));
        return nullptr;
    }
    
    URiveArtboard* Artboard = NewObject<URiveArtboard>();
    Artboard->Initialize(InRiveFile->GetNativeFile(), RiveRenderTarget, InArtboardName, InStateMachineName);    
    RenderObjects.Add(Artboard);
    
    return Artboard;
}

void URiveActorComponent::OnResourceInitialized_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef& NewResource) const
{
    // When the resource change, we need to tell the Render Target otherwise we will keep on drawing on an outdated RT
    if (const UE::Rive::Renderer::IRiveRenderTargetPtr RTarget = RiveRenderTarget) //todo: might need a lock
    {
        RTarget->CacheTextureTarget_RenderThread(RHICmdList, NewResource);
    }
}
