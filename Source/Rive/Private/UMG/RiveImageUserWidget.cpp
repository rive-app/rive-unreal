// Copyright Rive, Inc. All rights reserved.


#include "UMG/RiveImageUserWidget.h"

#include "Slate/SRiveImage.h"

#define LOCTEXT_NAMESPACE "RiveWidget"

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

void URiveImageUserWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

void URiveImageUserWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	RiveImage.Reset();
}

#if WITH_EDITOR
const FText URiveImageUserWidget::GetPaletteCategory()
{
	return LOCTEXT("Rive", "Rive");

}
#endif

TSharedRef<SWidget> URiveImageUserWidget::RebuildWidget()
{
	RiveImage = SNew(SRiveImage);
	RiveImage->RegisterArtboardInputs(Artboards);
	return RiveImage.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE