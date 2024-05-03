// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveWidget.h"

#include "Rive/RiveFile.h"
#include "Slate/SRiveWidget.h"

#define LOCTEXT_NAMESPACE "RiveWidget"

#if WITH_EDITOR

const FText URiveWidget::GetPaletteCategory()
{
    return LOCTEXT("Rive", "RiveUI");
}

#endif // WITH_EDITOR

void URiveWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);

    RiveWidget.Reset();
}

TSharedRef<SWidget> URiveWidget::RebuildWidget()
{
    RiveWidget = SNew(SRiveWidget);
    SetRiveFile(RiveFile);

    return RiveWidget.ToSharedRef();
}

void URiveWidget::SetAudioEngine(URiveAudioEngine* InAudioEngine)
{
    if (RiveFile)
    {
        RiveFile->SetAudioEngine(InAudioEngine);
        if (RiveFile->GetArtboard() != nullptr)
        {
            RiveFile->GetArtboard()->SetAudioEngine(InAudioEngine);
        }
    }
}

void URiveWidget::SetRiveFile(URiveFile* InRiveFile)
{
    if (RiveWidget.IsValid())
    {
        InRiveFile->InstantiateArtboard();
        RiveWidget->SetRiveFile(InRiveFile);
    }
}

#undef LOCTEXT_NAMESPACE