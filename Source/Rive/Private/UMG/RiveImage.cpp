// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveImage.h"

#include "Slate/SRiveImage.h"

#define LOCTEXT_NAMESPACE "URiveImage"

void URiveImage::SetRiveTexture(URiveTexture* InRiveTexture)
{
	if (!RiveImage)
	{
		return;
	}
	
	RiveImage->SetRiveTexture(InRiveTexture);
}

void URiveImage::RegisterArtboardInputs(const TArray<URiveArtboard*> InArtboards)
{
	Artboards = InArtboards;
    if (RiveImage)
    {
     	RiveImage->RegisterArtboardInputs(Artboards);
    }
}

void URiveImage::Setup(URiveTexture* InRiveTexture, const TArray<URiveArtboard*> InArtboards)
{
	SetRiveTexture(InRiveTexture);
	RegisterArtboardInputs(InArtboards);
}

void URiveImage::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	RiveImage.Reset();
}

#if WITH_EDITOR
const FText URiveImage::GetPaletteCategory()
{
	return LOCTEXT("Rive", "RiveUI");
}
#endif

TSharedRef<SWidget> URiveImage::RebuildWidget()
{
	RiveImage = SNew(SRiveImage);

	RiveImage->RegisterArtboardInputs(Artboards);

	return RiveImage.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE