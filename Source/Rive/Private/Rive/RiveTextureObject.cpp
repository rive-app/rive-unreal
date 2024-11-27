// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveTextureObject.h"
#include "IRiveRenderTarget.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Rive/RiveArtboard.h"
#include "Logs/RiveLog.h"
#include "Rive/Assets/RiveAsset.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Async/Async.h"
#include "RenderingThread.h"
#include "Rive/RiveFile.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/renderer/render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

URiveTextureObject::URiveTextureObject() {}

void URiveTextureObject::BeginDestroy()
{
#if WITH_EDITOR
    if (GIsEditor)
    {
        FEditorDelegates::BeginPIE.RemoveAll(this);
        FEditorDelegates::EndPIE.RemoveAll(this);
    }
#endif

    bIsRendering = false;
    OnRiveReady.Clear();
    RiveRenderTarget.Reset();

    if (IsValid(Artboard))
    {
        Artboard->MarkAsGarbage();
        Artboard = nullptr;
    }

    Super::BeginDestroy();
}

void URiveTextureObject::PostLoad()
{
    Super::PostLoad();

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        return;
    }

    if (!IsRunningCommandlet())
    {
        if (RiveDescriptor.RiveFile)
        {
#if WITH_EDITOR
            if (GIsEditor)
            {
                FEditorDelegates::BeginPIE.AddUObject(
                    this,
                    &URiveTextureObject::OnBeginPIE);
                FEditorDelegates::EndPIE.AddUObject(
                    this,
                    &URiveTextureObject::OnEndPIE);
            }
#endif
            Initialize(RiveDescriptor);
        }
    }
}

TStatId URiveTextureObject::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URiveFile, STATGROUP_Tickables);
}

void URiveTextureObject::Tick(float InDeltaSeconds)
{
    if (!IsValidChecked(this))
    {
        return;
    }

#if WITH_RIVE
    if (bIsRendering)
    {
        if (GetArtboard())
        {
            Artboard->Tick(InDeltaSeconds);
            RiveRenderTarget->SubmitAndClear();
        }
    }
#endif // WITH_RIVE
}

#if WITH_EDITOR
void URiveTextureObject::OnBeginPIE(bool bIsSimulating)
{
    Initialize(RiveDescriptor);
}

void URiveTextureObject::OnEndPIE(bool bIsSimulating) {}
#endif

FLinearColor URiveTextureObject::GetClearColor() const { return ClearColor; }

FVector2f URiveTextureObject::GetLocalCoordinate(URiveArtboard* InArtboard,
                                                 const FVector2f& InPosition)
{
#if WITH_RIVE
    if (InArtboard)
    {
        return InArtboard->GetLocalCoordinate(InPosition,
                                              Size,
                                              RiveDescriptor.Alignment,
                                              RiveDescriptor.FitType);
    }
#endif // WITH_RIVE
    return FVector2f::ZeroVector;
}

FVector2f URiveTextureObject::GetLocalCoordinatesFromExtents(
    const FVector2f& InPosition,
    const FBox2f& InExtents) const
{
#if WITH_RIVE
    if (GetArtboard())
    {
        return GetArtboard()->GetLocalCoordinatesFromExtents(
            InPosition,
            InExtents,
            Size,
            RiveDescriptor.Alignment,
            RiveDescriptor.FitType);
    }
#endif // WITH_RIVE
    return FVector2f::ZeroVector;
}

void URiveTextureObject::Initialize(const FRiveDescriptor& InRiveDescriptor)
{
    RiveDescriptor = InRiveDescriptor;

    if (!IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Could not load rive file as the required Rive Renderer "
                    "Module is either "
                    "missing or not loaded properly."));
        return;
    }

    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to instantiate the Artboard of Rive file '%s' as "
                    "we do not have a "
                    "valid renderer."),
               *GetFullNameSafe(this));
        return;
    }

    if (!RiveDescriptor.RiveFile)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Could not instance artboard as our native rive file is "
                    "invalid."));
        return;
    }

    RiveRenderer->CallOrRegister_OnInitialized(
        IRiveRenderer::FOnRendererInitialized::FDelegate::CreateUObject(
            this,
            &URiveTextureObject::OnRiveRendererInitialized));
}

void URiveTextureObject::OnRiveRendererInitialized(
    IRiveRenderer* InRiveRenderer)
{
    if (!RiveDescriptor.RiveFile->IsInitialized())
    {
        RiveDescriptor.RiveFile->OnInitializedDelegate.AddUObject(
            this,
            &URiveTextureObject::OnRiveFileInitialized);
    }
    else
    {
        OnRiveFileInitialized(true);
    }
}

void URiveTextureObject::OnResourceInitialized_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FTextureRHIRef& NewResource) const
{
    // When the resource change, we need to tell the Render Target otherwise we
    // will keep on drawing on an outdated RT
    if (const TSharedPtr<IRiveRenderTarget> RenderTarget =
            RiveRenderTarget) // todo: might need a lock
    {
        RenderTarget->CacheTextureTarget_RenderThread(RHICmdList, NewResource);
    }
}

void URiveTextureObject::OnRiveFileInitialized(bool bSuccess)
{
    if (!bSuccess)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("RiveTextureObject: RiveFile was not successfully "
                    "initialized."));
        return;
    }

    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        if (Artboard == nullptr)
            Artboard = NewObject<URiveArtboard>(this);
        else
            Artboard->Reinitialize(true);

        RiveRenderTarget.Reset();
        RiveRenderTarget =
            RiveRenderer->CreateTextureTarget_GameThread(GetFName(), this);

        if (!OnResourceInitializedOnRenderThread.IsBoundToObject(this))
        {
            OnResourceInitializedOnRenderThread.AddUObject(
                this,
                &URiveTextureObject::OnResourceInitialized_RenderThread);
        }

        RiveRenderTarget->SetClearColor(ClearColor);

        if (RiveDescriptor.ArtboardName.IsEmpty())
        {
            Artboard->Initialize(RiveDescriptor.RiveFile,
                                 RiveRenderTarget,
                                 RiveDescriptor.ArtboardIndex,
                                 RiveDescriptor.StateMachineName);
        }
        else
        {
            Artboard->Initialize(RiveDescriptor.RiveFile,
                                 RiveRenderTarget,
                                 RiveDescriptor.ArtboardName,
                                 RiveDescriptor.StateMachineName);
        }

        RiveDescriptor.ArtboardName = Artboard->GetArtboardName();
        RiveDescriptor.StateMachineName = Artboard->StateMachineName;

        if (Size == FIntPoint::ZeroValue)
        {
            ResizeRenderTargets(Artboard->GetSize());
        }
        else
        {
            ResizeRenderTargets(Size);
        }

        InitializeAudioEngine();

        Artboard->OnArtboardTick_Render.BindDynamic(
            this,
            &URiveTextureObject::OnArtboardTickRender);
        Artboard->OnGetLocalCoordinate.BindDynamic(
            this,
            &URiveTextureObject::GetLocalCoordinate);

        RiveRenderTarget->Initialize();
        bIsRendering = true;
        OnRiveReady.Broadcast();
    }
}

#if WITH_EDITOR
void URiveTextureObject::PostEditChangeChainProperty(
    FPropertyChangedChainEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    const FName ActiveMemberNodeName =
        *PropertyChangedEvent.PropertyChain.GetActiveMemberNode()
             ->GetValue()
             ->GetName();

    if (PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, RiveFile) ||
        PropertyName ==
            GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardIndex) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardName))
    {
        Initialize(RiveDescriptor);
    }
    else if (ActiveMemberNodeName ==
             GET_MEMBER_NAME_CHECKED(URiveTexture, Size))
    {
        ResizeRenderTargets(Size);
    }
    else if (ActiveMemberNodeName ==
             GET_MEMBER_NAME_CHECKED(URiveTextureObject, ClearColor))
    {
        if (RiveRenderTarget)
        {
            RiveRenderTarget->SetClearColor(ClearColor);
        }
    }

    // Update the Rive Cached RiveRenderTarget
    if (RiveRenderTarget)
    {
        RiveRenderTarget->Initialize();
    }

    FlushRenderingCommands();
}
#endif

void URiveTextureObject::OnArtboardTickRender(float DeltaTime,
                                              URiveArtboard* InArtboard)
{
    InArtboard->Align(RiveDescriptor.FitType,
                      RiveDescriptor.Alignment,
                      RiveDescriptor.ScaleFactor);
    InArtboard->Draw();
}

TArray<FString> URiveTextureObject::GetArtboardNamesForDropdown() const
{
    TArray<FString> Output;
    if (RiveDescriptor.RiveFile)
    {
        for (URiveArtboard* DescriptorArtboard :
             RiveDescriptor.RiveFile->Artboards)
        {
            Output.Add(DescriptorArtboard->GetArtboardName());
        }
    }

    return Output;
}

TArray<FString> URiveTextureObject::GetStateMachineNamesForDropdown() const
{
    TArray<FString> Output{""};
    if (RiveDescriptor.RiveFile)
    {
        for (URiveArtboard* RiveFileArtboard :
             RiveDescriptor.RiveFile->Artboards)
        {
            if (RiveFileArtboard->GetArtboardName().Equals(
                    RiveDescriptor.ArtboardName))
            {
                Output.Append(RiveFileArtboard->GetStateMachineNames());
                break;
            }
        }
    }
    return Output;
}

void URiveTextureObject::InitializeAudioEngine()
{
    if (RiveAudioEngine != nullptr && Artboard != nullptr)
    {
        if (RiveAudioEngine->GetNativeAudioEngine() == nullptr)
        {
            if (AudioEngineLambdaHandle.IsValid())
            {
                RiveAudioEngine->OnRiveAudioReady.Remove(
                    AudioEngineLambdaHandle);
                AudioEngineLambdaHandle.Reset();
            }

            TFunction<void()> AudioLambda = [this]() {
                Artboard->SetAudioEngine(RiveAudioEngine);
                RiveAudioEngine->OnRiveAudioReady.Remove(
                    AudioEngineLambdaHandle);
            };
            AudioEngineLambdaHandle =
                RiveAudioEngine->OnRiveAudioReady.AddLambda(AudioLambda);
        }
        else
        {
            Artboard->SetAudioEngine(RiveAudioEngine);
        }
    }
}

URiveArtboard* URiveTextureObject::GetArtboard() const
{
#if WITH_RIVE
    if (IsValid(Artboard) && Artboard->IsInitialized())
    {
        return Artboard;
    }
#endif // WITH_RIVE
    return nullptr;
}

void URiveTextureObject::SetAudioEngine(URiveAudioEngine* InRiveAudioEngine)
{
    RiveAudioEngine = InRiveAudioEngine;
    InitializeAudioEngine();
}
