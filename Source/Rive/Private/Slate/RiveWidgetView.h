// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"

class FRiveSceneViewport;
class FRiveViewportClient;
class URiveFile;
class FRiveSlateViewport;

/**
 *
 */
class RIVE_API SRiveWidgetView : public SCompoundWidget
{
public:

    SLATE_BEGIN_ARGS(SRiveWidgetView)
        {
        }

    SLATE_END_ARGS()

    //~ BEGIN : SWidget Interface

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

    //~ END : SWidget Interface

    /**
     * Implementation(s)
     */

public:

    void Construct(const FArguments& InArgs, URiveFile* InRiveFile);

    /** Get Parent slate window */
    TSharedPtr<SWindow> GetParentWindow() const;

	// SWidget overrides

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

    /**
     * Attribute(s)
     */

private:

    TObjectPtr<URiveFile> RiveFile;

    TSharedPtr<FRiveSlateViewport> RiveSlateViewport;

    /** Reference to Slate Viewport */
    TSharedPtr<SViewport> ViewportWidget;

	TSharedPtr<FRiveSceneViewport> RiveSceneViewport;
	
	TSharedPtr<FRiveViewportClient> RiveViewportClient;

	bool bIsRenderingEnabled = true;

    /** The slate window that contains this widget. This must be stored weak otherwise we create a circular reference. */
    mutable TWeakPtr<SWindow> SlateParentWindowPtr;
};
