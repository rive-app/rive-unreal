// Copyright Rive, Inc. All rights reserved.


#include "Slate/SRiveImage.h"

#include "Widgets/Images/SImage.h"
#include "SlateOptMacros.h"
#include "Rive/RiveTexture.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SRiveImage::Construct(const FArguments& InArgs, URiveTexture* InRiveTexture)
{
	RiveBrush = FSlateBrush();
	RiveBrush.DrawAs = ESlateBrushDrawType::Image;
	RiveBrush.SetResourceObject(InRiveTexture);

	ChildSlot
		[
			SNew(SImage)
			.Image(&RiveBrush)
		];
}

void SRiveImage::SetRiveTexture(URiveTexture* InRiveTexture)
{
	RiveBrush.SetResourceObject(InRiveTexture);
}

FReply SRiveImage::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnMouseMove %s"), *MouseEvent.GetScreenSpacePosition().ToString());
	
	return SCompoundWidget::OnMouseMove(MyGeometry, MouseEvent);
}

FReply SRiveImage::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnMouseButtonDown %s"), *MouseEvent.GetScreenSpacePosition().ToString());
	
	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SRiveImage::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnMouseButtonUp %s"), *MouseEvent.GetScreenSpacePosition().ToString());
	
	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SRiveImage::OnTouchMoved(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	return SCompoundWidget::OnTouchMoved(MyGeometry, InTouchEvent);
}

FReply SRiveImage::OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	return SCompoundWidget::OnTouchStarted(MyGeometry, InTouchEvent);
}

FReply SRiveImage::OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	return SCompoundWidget::OnTouchEnded(MyGeometry, InTouchEvent);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
