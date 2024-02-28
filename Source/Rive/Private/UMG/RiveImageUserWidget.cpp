// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveImageUserWidget.h"

#include "Slate/SRiveImage.h"

void URiveImageUserWidget::SetRiveTexture(URiveTexture* InRiveTexture)
{
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


TSharedRef<SWidget> URiveImageUserWidget::RebuildWidget()
{
	RiveImage = SNew(SRiveImage);
	RiveImage->RegisterArtboardInputs(Artboards);
	return RiveImage.ToSharedRef();
}

void URiveImageUserWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	RiveImage.Reset();
}

