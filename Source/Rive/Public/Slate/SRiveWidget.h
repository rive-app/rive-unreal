// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Engine/TimerHandle.h"
#include "Widgets/SCompoundWidget.h"

class SImage;
class URiveArtboard;
class FRiveStateMachine;
class URiveTexture;

/**
 *
 */

class RIVE_API SRiveWidget : public SCompoundWidget
{
public:
    DECLARE_DELEGATE_OneParam(FOnSizeChanged, const FVector2D&);

    SLATE_BEGIN_ARGS(SRiveWidget)
#if WITH_EDITOR
        :
        _bDrawCheckerboardInEditor(false)
#endif
    {}
#if WITH_EDITOR
    SLATE_ARGUMENT(bool, bDrawCheckerboardInEditor)
#endif
    SLATE_EVENT(FOnSizeChanged, OnSizeChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual void OnArrangeChildren(
        const FGeometry& AllottedGeometry,
        FArrangedChildren& ArrangedChildren) const override;

    void SetRiveTexture(URiveTexture* InRiveTexture);
    FVector2D GetSize();

private:
    UWorld* GetWorld() const;
    void OnResize() const;

    URiveTexture* RiveTexture = nullptr;
    TArray<URiveArtboard*> Artboards;

    TSharedPtr<SImage> RiveImageView;
    TSharedPtr<FSlateBrush> RiveTextureBrush;

    double LastSizeChangeTime = 0;
    mutable FTimerHandle TimerHandle;
    mutable FVector2D PreviousSize;

    FOnSizeChanged OnSizeChangedDelegate;

#if WITH_EDITOR // Implementation of Checkerboard textures, as per
                // FTextureEditorViewportClient::ModifyCheckerboardTextureColors
    /** Modifies the checkerboard texture's data */
    void ModifyCheckerboardTextureColors();
    /** Destroy the checkerboard texture if one exists */
    void DestroyCheckerboardTexture();
    /** Checkerboard texture */
    TObjectPtr<UTexture2D> CheckerboardTexture;
    FSlateBrush* CheckerboardBrush;
#endif
};
