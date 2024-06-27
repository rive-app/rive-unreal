// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveWidget.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveTextureObject.h"
#include "Slate/SRiveWidget.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "RiveWidget"

URiveWidget::~URiveWidget()
{
	if (RiveWidget != nullptr)
	{
		RiveWidget->SetRiveTexture(nullptr);
	}

	RiveWidget.Reset();

	if (RiveObject != nullptr)
	{
		RiveObject->MarkAsGarbage();
		RiveObject = nullptr;
	}
}

#if WITH_EDITOR

const FText URiveWidget::GetPaletteCategory()
{
	return LOCTEXT("Rive", "RiveUI");
}

#endif // WITH_EDITOR

void URiveWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	if (RiveWidget != nullptr)
	{
		RiveWidget->SetRiveTexture(nullptr);
	}

	RiveWidget.Reset();

	if (RiveObject != nullptr)
	{
		RiveObject->MarkAsGarbage();
		RiveObject = nullptr;
	}
}

TSharedRef<SWidget> URiveWidget::RebuildWidget()
{
	RiveWidget = SNew(SRiveWidget);
	
	if (!RiveObject && RiveWidget.IsValid())
	{
		RiveObject = NewObject<URiveTextureObject>();

#if WITH_EDITOR
		TimerHandle.Invalidate();
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			Setup();
		}, 0.05f, false);
#else
		Setup();
#endif
	}
	
	return RiveWidget.ToSharedRef();
}

void URiveWidget::SetAudioEngine(URiveAudioEngine* InAudioEngine)
{
	if (RiveObject && RiveObject->GetArtboard())
	{
		RiveObject->GetArtboard()->SetAudioEngine(InAudioEngine);
		return;
	}
	
	UE_LOG(LogRive, Warning, TEXT("RiveObject was null while trying to SetAudioEngine"));
}

URiveArtboard* URiveWidget::GetArtboard() const
{
	if (RiveObject && RiveObject->GetArtboard())
	{
		return RiveObject->GetArtboard();
	}
	
	return nullptr;
}

void URiveWidget::OnRiveObjectReady()
{
	if (!RiveWidget.IsValid() || !GetCachedWidget()) return;
	RiveObject->OnRiveReady.Remove(FrameHandle);
		
	UE::Slate::FDeprecateVector2DResult AbsoluteSize = GetCachedGeometry().GetAbsoluteSize();

	RiveObject->ResizeRenderTargets(FIntPoint(AbsoluteSize.X, AbsoluteSize.Y));
	RiveWidget->SetRiveTexture(RiveObject);
	RiveWidget->RegisterArtboardInputs({RiveObject->GetArtboard()});
	OnRiveReady.Broadcast();
}

void URiveWidget::Setup()
{
	if (!RiveObject || !RiveWidget.IsValid())
	{
		return;
	}
	
	FrameHandle = RiveObject->OnRiveReady.AddUObject(this, &URiveWidget::OnRiveObjectReady);
#if WITH_EDITOR
	RiveObject->bRenderInEditor = true;
#endif
	RiveObject->Initialize(RiveDescriptor);
}

#undef LOCTEXT_NAMESPACE
