// Copyright Rive, Inc. All rights reserved.

#include "RiveWidgetView.h"
#include "RiveSlateViewport.h"
#include "Widgets/SViewport.h"

void SRiveWidgetView::Construct(const FArguments& InArgs, URiveFile* InRiveFile)
{
    RiveFile = InRiveFile;

    RiveSlateViewport = MakeShared<FRiveSlateViewport>(InRiveFile, SharedThis(this));

    TSharedRef<SViewport> ViewportRef =
        SNew(SViewport)
        .EnableGammaCorrection(false)
        .EnableBlending(true)
        .IgnoreTextureAlpha(false);

    Viewport = ViewportRef;

    ChildSlot
        [
            ViewportRef
        ];

    Viewport->SetViewportInterface(RiveSlateViewport.ToSharedRef());
}

TSharedPtr<SWindow> SRiveWidgetView::GetParentWindow() const
{
    return SlateParentWindowPtr.Pin();
}

int32 SRiveWidgetView::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    int32 Layer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    // Cache a reference to our parent window, if we didn't already reference it.
    if (!SlateParentWindowPtr.IsValid())
    {
        SWindow* ParentWindow = OutDrawElements.GetPaintWindow();

        TSharedRef<SWindow> SlateParentWindowRef = StaticCastSharedRef<SWindow>(ParentWindow->AsShared());

        SlateParentWindowPtr = SlateParentWindowRef;
    }

    return Layer;
}
