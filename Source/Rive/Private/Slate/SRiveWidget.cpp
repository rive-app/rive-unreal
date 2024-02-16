// Copyright Rive, Inc. All rights reserved.

#include "Slate/SRiveWidget.h"
#include "RiveWidgetView.h"
#include "Rive/RiveFile.h"

void SRiveWidget::Construct(const FArguments& InArgs, URiveFile* InRiveFile)
{
    ChildSlot
        [
            SNew(SVerticalBox)

                + SVerticalBox::Slot()
                [
                    SAssignNew(RiveWidgetView, SRiveWidgetView, InRiveFile)
#if WITH_EDITOR
                    .bDrawCheckerboardInEditor(InArgs._bDrawCheckerboardInEditor)
#endif
                ]
        ];
}

void SRiveWidget::SetRiveFile(URiveFile* InRiveFile)
{
    RiveFile = InRiveFile;
}
