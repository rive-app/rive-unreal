// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "Engine/TimerHandle.h"
#include "Rive/RiveDescriptor.h"
#include "Widgets/SCompoundWidget.h"

class SRiveLeafWidget;
class SImage;
class URiveArtboard;
struct FRiveStateMachine;
class URiveTexture;

/**
 *
 */

class RIVE_API SRiveWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SRiveWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual void OnArrangeChildren(
        const FGeometry& AllottedGeometry,
        FArrangedChildren& ArrangedChildren) const override;

    void SetRiveArtboard(URiveArtboard* InRiveArtboard);
    void SetRiveDescriptor(const FRiveDescriptor& InRiveArtboard);
    FVector2D GetSize();

private:
    UWorld* GetWorld() const;

    TWeakObjectPtr<URiveArtboard> Artboard;
    FRiveDescriptor RiveDescriptor;
    TSharedPtr<SRiveLeafWidget> RiveImageView;
};
