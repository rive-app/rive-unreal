// Copyright Rive, Inc. All rights reserved.

#include "RiveWidgetView.h"
#include "Widgets/SViewport.h"

#include "RiveSceneViewport.h"
#include "RiveViewportClient.h"
#include "Rive/RiveTexture.h"
#include "RiveArtboard.h"

void SRiveWidgetView::Construct(const FArguments& InArgs, URiveTexture* InRiveTexture, const TArray<URiveArtboard*>& InArtboards)
{
    RiveTexture = InRiveTexture;
    
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


    RiveViewportClient = MakeShared<FRiveViewportClient>(InRiveTexture, InArtboards,  SharedThis(this));
#if WITH_EDITOR
    RiveViewportClient->bDrawCheckeredTexture = InArgs._bDrawCheckerboardInEditor;
#endif
    
    RiveSceneViewport = MakeShared<FRiveSceneViewport>(RiveViewportClient.Get(), ViewportWidget, InRiveTexture, InArtboards);
    
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

void SRiveWidgetView::SetRiveTexture(URiveTexture* InRiveTexture)
{
    RiveViewportClient->SetRiveTexture(InRiveTexture);
    RiveSceneViewport->SetRiveTexture(InRiveTexture);
}

void SRiveWidgetView::RegisterArtboardInputs(const TArray<URiveArtboard*>& InArtboards)
{
    RiveViewportClient->RegisterArtboardInputs(InArtboards);
    RiveSceneViewport->RegisterArtboardInputs(InArtboards);
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
