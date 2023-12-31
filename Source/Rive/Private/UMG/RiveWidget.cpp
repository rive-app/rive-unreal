// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveWidget.h"
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
    RiveWidget = SNew(SRiveWidget, RiveFile);

    return RiveWidget.ToSharedRef();
}

void URiveWidget::SetRiveFile(URiveFile* InRiveFile)
{
    RiveFile = InRiveFile;

    if (RiveWidget.IsValid())
    {
        RiveWidget->SetRiveFile(RiveFile);
    }
}

#undef LOCTEXT_NAMESPACE