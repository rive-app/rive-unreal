// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "Slate/SRiveWidget.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#endif
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "ImageUtils.h"
#include "TimerManager.h"
#include "Slate/SRiveLeafWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Rive/RiveArtboard.h"

#if WITH_EDITOR
#include "TextureEditorSettings.h"
#endif

namespace UE::Private::SRiveWidget
{
FSlateBrush* CreateTransparentBrush()
{
    FSlateBrush* TransparentBrush = new FSlateBrush();
    TransparentBrush->DrawAs = ESlateBrushDrawType::NoDrawType;
    return TransparentBrush;
}
} // namespace UE::Private::SRiveWidget

void SRiveWidget::Construct(const FArguments& InArgs)
{
    ChildSlot[SNew(SOverlay) +
              SOverlay::Slot()[SAssignNew(RiveImageView, SRiveLeafWidget)
                                   .Artboard(Artboard.Get())
                                   .Descriptor(RiveDescriptor)]];
}

void SRiveWidget::OnArrangeChildren(const FGeometry& AllottedGeometry,
                                    FArrangedChildren& ArrangedChildren) const
{
    SCompoundWidget::OnArrangeChildren(AllottedGeometry, ArrangedChildren);
}

void SRiveWidget::SetRiveArtboard(URiveArtboard* InRiveArtboard)
{
    if (RiveImageView)
    {
        RiveImageView->SetArtboard(InRiveArtboard);
    }
}

void SRiveWidget::SetRiveDescriptor(const FRiveDescriptor& InRiveArtboard)
{
    if (RiveImageView)
    {
        RiveImageView->SetRiveDescriptor(InRiveArtboard);
    }
}

UWorld* SRiveWidget::GetWorld() const
{
#if WITH_EDITOR
    if (GEditor && GEditor->PlayWorld)
    {
        return GEditor->PlayWorld;
    }

    if (GEditor && GEditor->GetEditorWorldContext().World())
    {
        return GEditor->GetEditorWorldContext().World();
    }
#endif

    if (GEngine && GEngine->GetWorld())
    {
        return GEngine->GetWorld();
    }

    return nullptr;
}
