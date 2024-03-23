// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveImageUserWidget.h"

#include "Slate/SRiveWidget.h"

void URiveImageUserWidget::SetRiveTexture(URiveTexture* InRiveTexture)
{
	RiveTexture = InRiveTexture;

	if (!RiveWidget)
	{
		return;
	}
	
	RiveWidget->SetRiveTexture(InRiveTexture);
}

void URiveImageUserWidget::RegisterArtboardInputs(const TArray<URiveArtboard*> InArtboards)
{
	Artboards = InArtboards;
	if (RiveWidget)
	{
		RiveWidget->RegisterArtboardInputs(Artboards);
	}
}

void URiveImageUserWidget::Setup(URiveTexture* InRiveTexture, const TArray<URiveArtboard*> InArtboards)
{
	SetRiveTexture(InRiveTexture);
	RegisterArtboardInputs(InArtboards);
}

bool URiveImageUserWidget::Initialize()
{
	const bool bParentReturn = Super::Initialize();

	RiveWidget = SNew(SRiveWidget);

	return bParentReturn;
}

TSharedRef<SWidget> URiveImageUserWidget::RebuildWidget()
{
	if (!RiveWidget.IsValid())
	{
		RiveWidget = SNew(SRiveWidget);
	}
	Setup(RiveTexture, Artboards);
	return RiveWidget.ToSharedRef();
}

void URiveImageUserWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	RiveWidget.Reset();
}

