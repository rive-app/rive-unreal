// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveWidget.h"
#include "Rive/RiveObject.h"
#include "Slate/SRiveWidget.h"

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
	Setup();
	return RiveWidget.ToSharedRef();
}

void URiveWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Setup();
}

void URiveWidget::SetAudioEngine(URiveAudioEngine* InAudioEngine)
{
	RiveObject->GetArtboard()->SetAudioEngine(InAudioEngine);
}

void URiveWidget::Setup()
{
	if (!RiveObject)
	{
		TimerHandle.Invalidate();
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			RiveObject = NewObject<URiveObject>();
			RiveObject->OnRiveReady.AddLambda([this]()
			{
				UE::Slate::FDeprecateVector2DResult AbsoluteSize = GetCachedGeometry().GetAbsoluteSize();

				RiveObject->ResizeRenderTargets(FIntPoint(AbsoluteSize.X, AbsoluteSize.Y));
				RiveWidget->SetRiveTexture(RiveObject);
				RiveWidget->RegisterArtboardInputs({RiveObject->GetArtboard()});
				OnRiveReady.Broadcast();
			});
			RiveObject->Initialize(RiveDescriptor);
		}, 0.05f, false);
	}
}

#undef LOCTEXT_NAMESPACE
