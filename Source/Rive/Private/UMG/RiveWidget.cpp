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

	if (RiveTextureObject != nullptr)
	{
		RiveTextureObject->MarkAsGarbage();
		RiveTextureObject = nullptr;
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

	if (RiveTextureObject != nullptr)
	{
		RiveTextureObject->MarkAsGarbage();
		RiveTextureObject = nullptr;
	}
}

TSharedRef<SWidget> URiveWidget::RebuildWidget()
{
	RiveWidget = SNew(SRiveWidget);
	
	if (!RiveTextureObject && RiveWidget.IsValid())
	{
		RiveTextureObject = NewObject<URiveTextureObject>();

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
	if (RiveTextureObject && RiveTextureObject->GetArtboard())
	{
		RiveTextureObject->GetArtboard()->SetAudioEngine(InAudioEngine);
		return;
	}
	
	UE_LOG(LogRive, Warning, TEXT("RiveObject was null while trying to SetAudioEngine"));
}

URiveArtboard* URiveWidget::GetArtboard() const
{
	if (RiveTextureObject && RiveTextureObject->GetArtboard())
	{
		return RiveTextureObject->GetArtboard();
	}
	
	return nullptr;
}

void URiveWidget::OnRiveObjectReady()
{
	if (!RiveWidget.IsValid() || !GetCachedWidget()) return;
	RiveTextureObject->OnRiveReady.Remove(FrameHandle);
		
	UE::Slate::FDeprecateVector2DResult AbsoluteSize = GetCachedGeometry().GetAbsoluteSize();

	RiveTextureObject->ResizeRenderTargets(FIntPoint(AbsoluteSize.X, AbsoluteSize.Y));
	RiveWidget->SetRiveTexture(RiveTextureObject);
	RiveWidget->RegisterArtboardInputs({RiveTextureObject->GetArtboard()});
	OnRiveReady.Broadcast();
}

void URiveWidget::Setup()
{
	if (!RiveTextureObject || !RiveWidget.IsValid())
	{
		return;
	}
	
	FrameHandle = RiveTextureObject->OnRiveReady.AddUObject(this, &URiveWidget::OnRiveObjectReady);
#if WITH_EDITOR
	RiveTextureObject->bRenderInEditor = true;
#endif
	RiveTextureObject->Initialize(RiveDescriptor);
}

#undef LOCTEXT_NAMESPACE
