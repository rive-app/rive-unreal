// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveObject.h"
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
#include "PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/renderer/render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

URiveObject::URiveObject()
{
}

void URiveObject::BeginDestroy()
{
	OnRiveReady.Clear();
	RiveRenderTarget.Reset();

	if (IsValid(Artboard))
	{
		Artboard->MarkAsGarbage();
	}

	Super::BeginDestroy();
}

TStatId URiveObject::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URiveFile, STATGROUP_Tickables);
}

void URiveObject::Tick(float InDeltaSeconds)
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

bool URiveObject::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && bIsRendering;
}

FVector2f URiveObject::GetLocalCoordinate(URiveArtboard* InArtboard, const FVector2f& InPosition)
{
#if WITH_RIVE
	if (InArtboard)
	{
		return InArtboard->GetLocalCoordinate(InPosition, Size, RiveDescriptor.Alignment, RiveDescriptor.FitType);
	}
#endif // WITH_RIVE
	return FVector2f::ZeroVector;
}

FVector2f URiveObject::GetLocalCoordinatesFromExtents(const FVector2f& InPosition, const FBox2f& InExtents) const
{
#if WITH_RIVE
	if (GetArtboard())
	{
		return GetArtboard()->GetLocalCoordinatesFromExtents(InPosition, InExtents, Size, RiveDescriptor.Alignment, RiveDescriptor.FitType);
	}
#endif // WITH_RIVE
	return FVector2f::ZeroVector;
}

void URiveObject::Initialize(const FRiveDescriptor& InRiveDescriptor)
{
	Artboard = nullptr;
	RiveDescriptor = InRiveDescriptor;

	if (!IRiveRendererModule::IsAvailable())
	{
		UE_LOG(LogRive, Error, TEXT("Could not load rive file as the required Rive Renderer Module is either missing or not loaded properly."));
		return;
	}

	IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

	if (!RiveRenderer)
	{
		UE_LOG(LogRive, Error, TEXT("Failed to instantiate the Artboard of Rive file '%s' as we do not have a valid renderer."), *GetFullNameSafe(this));
		return;
	}

	if (!RiveDescriptor.RiveFile)
	{
		UE_LOG(LogRive, Error, TEXT("Could not instance artboard as our native rive file is invalid."));
		return;
	}

	RiveRenderer->CallOrRegister_OnInitialized(IRiveRenderer::FOnRendererInitialized::FDelegate::CreateUObject(this, &URiveObject::RiveReady));
}

void URiveObject::RiveReady(IRiveRenderer* InRiveRenderer)
{
	Artboard = NewObject<URiveArtboard>(this);
	RiveDescriptor.RiveFile->Artboards.Add(Artboard);
	RiveRenderTarget.Reset();
	RiveRenderTarget = InRiveRenderer->CreateTextureTarget_GameThread(GetFName(), this);
			
	if (!OnResourceInitializedOnRenderThread.IsBoundToObject(this))
	{
		OnResourceInitializedOnRenderThread.AddUObject(this, &URiveObject::OnResourceInitialized_RenderThread);
	}
			
	RiveRenderTarget->SetClearColor(ClearColor);

	if (RiveDescriptor.ArtboardName.IsEmpty())
	{
		Artboard->Initialize(RiveDescriptor.RiveFile->GetNativeFile(), RiveRenderTarget, RiveDescriptor.ArtboardIndex, RiveDescriptor.StateMachineName);
	}
	else
	{
		Artboard->Initialize(RiveDescriptor.RiveFile->GetNativeFile(), RiveRenderTarget, RiveDescriptor.ArtboardName, RiveDescriptor.StateMachineName);
	}

	ResizeRenderTargets(Artboard->GetSize());

	if (AudioEngine != nullptr)
	{
		if (AudioEngine->GetNativeAudioEngine() == nullptr)
		{
			if (AudioEngineLambdaHandle.IsValid())
			{
				AudioEngine->OnRiveAudioReady.Remove(AudioEngineLambdaHandle);
				AudioEngineLambdaHandle.Reset();
			}

			TFunction<void()> AudioLambda = [this]()
			{
				Artboard->SetAudioEngine(AudioEngine);
				AudioEngine->OnRiveAudioReady.Remove(AudioEngineLambdaHandle);
			};
			AudioEngineLambdaHandle = AudioEngine->OnRiveAudioReady.AddLambda(AudioLambda);
		}
		else
		{
			Artboard->SetAudioEngine(AudioEngine);
		}
	}
	
	Artboard->OnArtboardTick_Render.BindDynamic(this, &URiveObject::OnArtboardTickRender);
	Artboard->OnGetLocalCoordinate.BindDynamic(this, &URiveObject::GetLocalCoordinate);
	
	RiveRenderTarget->Initialize();
	OnRiveReady.Broadcast();
}

void URiveObject::OnResourceInitialized_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef& NewResource) const
{
	// When the resource change, we need to tell the Render Target otherwise we will keep on drawing on an outdated RT
	if (const TSharedPtr<IRiveRenderTarget> RenderTarget = RiveRenderTarget) //todo: might need a lock
	{
		RenderTarget->CacheTextureTarget_RenderThread(RHICmdList, NewResource);
	}
}

void URiveObject::OnArtboardTickRender(float DeltaTime, URiveArtboard* InArtboard)
{
	InArtboard->Align(RiveDescriptor.FitType, RiveDescriptor.Alignment);
	InArtboard->Draw();
}

URiveArtboard* URiveObject::GetArtboard() const
{
#if WITH_RIVE
	if (IsValid(Artboard) && Artboard->IsInitialized())
	{
		return Artboard;
	}
#endif // WITH_RIVE
	return nullptr;
}
