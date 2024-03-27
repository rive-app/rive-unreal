// Copyright Rive, Inc. All rights reserved.

#include "Slate/SRiveWidget.h"

#include "RiveWidgetView.h"
#include "Rive/RiveFile.h"
#include "Components/VerticalBox.h"

SRiveWidget::~SRiveWidget()
{
    if (IsValid(RiveFile))
    {
        RiveFile->OnArtboardChangedRaw.Remove(OnArtboardChangedHandle);
    }
}

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

void SRiveWidget::SetRiveFile(URiveFile* InRiveFile)
{
    if (!RiveWidgetView || InRiveFile == RiveFile)
    {
        return;
    }

    if (IsValid(RiveFile))
    {
        RiveFile->OnArtboardChangedRaw.Remove(OnArtboardChangedHandle);
    }
    
    if (IsValid(InRiveFile))
    {
        RiveFile = InRiveFile;
        RiveWidgetView->SetRiveTexture(RiveFile);
        if (URiveArtboard* Artboard = RiveFile->GetArtboard())
        {
            RiveWidgetView->RegisterArtboardInputs({ Artboard });
        }
        else
        {
            RiveWidgetView->RegisterArtboardInputs({});
        }

        TWeakPtr<SRiveWidget> WeakRiveWidget = SharedThis(this).ToWeakPtr();
        OnArtboardChangedHandle = InRiveFile->OnArtboardChangedRaw.AddSPLambda(this, [WeakRiveWidget, InRiveFile](URiveFile* File, URiveArtboard* Artboard)
        {
            if (const TSharedPtr<SRiveWidget> RiveWidget = WeakRiveWidget.Pin())
            {
                if (ensure(InRiveFile == File) && RiveWidget->RiveWidgetView)
                {
                    if (Artboard)
                    {
                        RiveWidget->RiveWidgetView->RegisterArtboardInputs({ Artboard });
                    }
                    else
                    {
                        RiveWidget->RiveWidgetView->RegisterArtboardInputs({});
                    }
                }
            }
        });
    }
    else
    {
        RiveFile = nullptr;
        RiveWidgetView->SetRiveTexture(RiveFile);
        RiveWidgetView->RegisterArtboardInputs({});
    }
}
