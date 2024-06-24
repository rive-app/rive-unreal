// Copyright Rive, Inc. All rights reserved.

#include "Slate/SRiveWidget.h"

#include "RiveWidgetView.h"
#include "Rive/RiveObject.h"


void SRiveWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                [
                    SAssignNew(RiveWidgetView, SRiveWidgetView)
#if WITH_EDITOR
                    .bDrawCheckerboardInEditor(InArgs._bDrawCheckerboardInEditor)
#endif
                ]
        ];
}

void SRiveWidget::SetRiveTexture(URiveTexture* InRiveTexture)
{
    if (RiveWidgetView)
    {
        RiveWidgetView->SetRiveTexture(InRiveTexture);
    }
}

void SRiveWidget::RegisterArtboardInputs(const TArray<URiveArtboard*>& InArtboards)
{
    if (RiveWidgetView)
    {
        RiveWidgetView->RegisterArtboardInputs(InArtboards);
    }
}
