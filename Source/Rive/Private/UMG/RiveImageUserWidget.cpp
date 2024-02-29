// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveImageUserWidget.h"

#include "Slate/SRiveImage.h"

void URiveImageUserWidget::SetRiveTexture(URiveTexture* InRiveTexture)
{
	RiveTexture = InRiveTexture;

	if (!RiveImage)
	{
		return;
	}
	
	RiveImage->SetRiveTexture(InRiveTexture);
}

void URiveImageUserWidget::RegisterArtboardInputs(const TArray<URiveArtboard*> InArtboards)
{
	Artboards = InArtboards;
	if (RiveImage)
	{
		RiveImage->RegisterArtboardInputs(Artboards);
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

	RiveImage = SNew(SRiveImage);

	return bParentReturn;
}

TSharedRef<SWidget> URiveImageUserWidget::RebuildWidget()
{
	if (!RiveImage.IsValid())
	{
		RiveImage = SNew(SRiveImage);
	}
	return RiveImage.ToSharedRef();
}

void URiveImageUserWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	RiveImage.Reset();
}

