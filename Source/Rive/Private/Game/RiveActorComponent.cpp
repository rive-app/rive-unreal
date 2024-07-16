// Copyright Rive, Inc. All rights reserved.

#include "Game/RiveActorComponent.h"

#include <Rive/RiveTexture.h>

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Rive/RiveArtboard.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveDescriptor.h"
#include "Rive/RiveFile.h"
#include "Stats/RiveStats.h"

class FRiveStateMachine;

URiveActorComponent::URiveActorComponent(): Size(500, 500)
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;
}

void URiveActorComponent::BeginPlay()
{
    Initialize();
    Super::BeginPlay();
}

void URiveActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsValidChecked(this))
    {
        return;
    }

    SCOPED_NAMED_EVENT_TEXT(TEXT("URiveActorComponent::TickComponent"), FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("URiveActorComponent::TickComponent"), STAT_RIVEACTORCOMPONENT_TICK, STATGROUP_Rive);

    if (RiveRenderTarget)
    {
        for (URiveArtboard* Artboard : Artboards)
        {
            RiveRenderTarget->Save();
            Artboard->Tick(DeltaTime);
            RiveRenderTarget->Restore();
        }


        RiveRenderTarget->SubmitAndClear();
    }
}

void URiveActorComponent::Initialize()
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive, Error, TEXT("RiveRenderer is null, unable to initialize the RenderTarget for Rive file '%s'"), *GetFullNameSafe(this));
        return;
    }
    
    RiveRenderer->CallOrRegister_OnInitialized(IRiveRenderer::FOnRendererInitialized::FDelegate::CreateUObject(this, &URiveActorComponent::RiveReady));
}

void URiveActorComponent::ResizeRenderTarget(int32 InSizeX, int32 InSizeY)
{
    if (!RiveTexture)
    {
        return;
    }
	
    RiveTexture->ResizeRenderTargets(FIntPoint(InSizeX, InSizeY));
}

URiveArtboard* URiveActorComponent::AddArtboard(URiveFile* InRiveFile, const FString& InArtboardName, const FString& InStateMachineName)
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
	
    if (!IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer Module is either missing or not loaded properly."));
        return nullptr;
    }

    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

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
    Artboard->Initialize(InRiveFile, RiveRenderTarget, InArtboardName, InStateMachineName);    
    Artboards.Add(Artboard);

    if (RiveAudioEngine != nullptr)
    {
        Artboard->SetAudioEngine(RiveAudioEngine);
    }
    
    return Artboard;
}

void URiveActorComponent::RemoveArtboard(URiveArtboard* InArtboard)
{
    Artboards.RemoveSingle(InArtboard);
}

URiveArtboard* URiveActorComponent::GetDefaultArtboard() const
{
    return GetArtboardAtIndex(0);
}

URiveArtboard* URiveActorComponent::GetArtboardAtIndex(int32 InIndex) const
{
    if (Artboards.IsEmpty())
    {
        return nullptr;
    }

    if (InIndex >= Artboards.Num())
    {
        UE_LOG(LogRive, Warning, TEXT("GetArtboardAtIndex with index %d is out of bounds"), InIndex);
        return nullptr;
    }

    return Artboards[InIndex];
}

int32 URiveActorComponent::GetArtboardCount() const
{
    return Artboards.Num();
}

void URiveActorComponent::OnResourceInitialized_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef& NewResource)
{
    // When the resource change, we need to tell the Render Target otherwise we will keep on drawing on an outdated RT
    if (const TSharedPtr<IRiveRenderTarget> RTarget = RiveRenderTarget) //todo: might need a lock
    {
        RTarget->CacheTextureTarget_RenderThread(RHICmdList, NewResource);
    }
}

void URiveActorComponent::OnDefaultArtboardTickRender(float DeltaTime, URiveArtboard* InArtboard)
{
    InArtboard->Align(DefaultRiveDescriptor.FitType, DefaultRiveDescriptor.Alignment);
    InArtboard->Draw();
}

void URiveActorComponent::RiveReady(IRiveRenderer* InRiveRenderer)
{
    RiveTexture = NewObject<URiveTexture>();
    // Initialize Rive Render Target Only after we resize the texture
    RiveRenderTarget = InRiveRenderer->CreateTextureTarget_GameThread(GetFName(), RiveTexture);
    RiveRenderTarget->SetClearColor(FLinearColor::Transparent);
    RiveTexture->ResizeRenderTargets(FIntPoint(Size.X, Size.Y));
    RiveRenderTarget->Initialize();

    RiveTexture->OnResourceInitializedOnRenderThread.AddUObject(this, &URiveActorComponent::OnResourceInitialized_RenderThread);
    
    if (DefaultRiveDescriptor.RiveFile)
    {
        URiveArtboard* Artboard = AddArtboard(DefaultRiveDescriptor.RiveFile, DefaultRiveDescriptor.ArtboardName, DefaultRiveDescriptor.StateMachineName);
        Artboard->OnArtboardTick_Render.BindDynamic(this, &URiveActorComponent::OnDefaultArtboardTickRender);
    } 
    
    OnRiveReady.Broadcast();
}
