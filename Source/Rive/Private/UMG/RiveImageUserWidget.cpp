// Copyright Rive, Inc. All rights reserved.


#include "UMG/RiveImageUserWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Slate/RiveWidgetHelpers.h"
#include "UMG/RiveImage.h"

bool URiveImageUserWidget::Initialize()
{
	return Super::Initialize();
}

TSharedRef<SWidget> URiveImageUserWidget::RebuildWidget()
{
	RootCanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = RootCanvasPanel;
	RiveImage = WidgetTree->ConstructWidget<URiveImage>(URiveImage::StaticClass());
	RiveImageSlot = RootCanvasPanel->AddChildToCanvas(RiveImage);
	
	return RootCanvasPanel->TakeWidget();
}

void URiveImageUserWidget::CalculateCenterPlacementInViewport(const FVector2f& TextureSize, const FVector2f& InViewportSize, FVector2f& OutPosition, FVector2f& OutSize)
{
	const FBox2f Box = RiveWidgetHelpers::CalculateRenderTextureExtentsInViewport(TextureSize, InViewportSize);
	OutPosition = Box.Min;
	OutSize = Box.GetSize();
}

