// Copyright Rive, Inc. All rights reserved.

#include "RiveWidgetView.h"
#include "Widgets/SViewport.h"

#include "RiveSceneViewport.h"
#include "RiveViewportClient.h"
#include "Rive/RiveFile.h"

void SRiveWidgetView::Construct(const FArguments& InArgs, URiveFile* InRiveFile)
{
    RiveFile = InRiveFile;

    if (!RiveFile)
    {
        return;
    }
    
    TSharedRef<SViewport> ViewportRef =
        SNew(SViewport)
        .EnableGammaCorrection(false)
        .EnableBlending(true)
        .IgnoreTextureAlpha(false);

    ViewportWidget = ViewportRef;

    ChildSlot
        [
            ViewportRef
        ];


    RiveViewportClient = MakeShared<FRiveViewportClient>(InRiveFile, SharedThis(this));
#if WITH_EDITOR
    RiveViewportClient->bDrawCheckeredTexture = InArgs._bDrawCheckerboardInEditor;
#endif
    
    RiveSceneViewport = MakeShared<FRiveSceneViewport>(RiveViewportClient.Get(), ViewportWidget, InRiveFile);
    
    ViewportWidget->SetViewportInterface(RiveSceneViewport.ToSharedRef());
}

TSharedPtr<SWindow> SRiveWidgetView::GetParentWindow() const
{
    return SlateParentWindowPtr.Pin();
}

void SRiveWidgetView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    if (bIsRenderingEnabled)
    {
        if (RiveSceneViewport.IsValid())
        {
            RiveSceneViewport->Invalidate();
        }
    }
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
