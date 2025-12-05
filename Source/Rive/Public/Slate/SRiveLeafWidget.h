// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "Engine/TimerHandle.h"
#include "Rive/RiveDescriptor.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

class SImage;
class URiveArtboard;
struct FRiveStateMachine;
class UUserWidget;
class URiveTexture;

class RIVE_API SRiveLeafWidget : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRiveLeafWidget) {}
    SLATE_ATTRIBUTE(TObjectPtr<UUserWidget>, OwningWidget)
    SLATE_ATTRIBUTE(TWeakObjectPtr<URiveArtboard>, Artboard)
    SLATE_ATTRIBUTE(FRiveDescriptor, Descriptor)
    SLATE_END_ARGS()

    void SetRiveDescriptor(const FRiveDescriptor& InDescriptor);

    void SetArtboard(TWeakObjectPtr<URiveArtboard> InArtboard);

    void Construct(const FArguments& InArgs);
    virtual int32 OnPaint(const FPaintArgs& Args,
                          const FGeometry& AllottedGeometry,
                          const FSlateRect& MyCullingRect,
                          FSlateWindowElementList& OutDrawElements,
                          int32 LayerId,
                          const FWidgetStyle& InWidgetStyle,
                          bool bParentEnabled) const override;

    virtual FVector2D ComputeDesiredSize(float) const override;

private:
    TObjectPtr<UUserWidget> OwningWidget;
    TWeakObjectPtr<URiveArtboard> Artboard;
    TSharedPtr<class FRiveRendererDrawElement> RiveRendererDrawElement;

    bool bScaleByDPI = false;
};
