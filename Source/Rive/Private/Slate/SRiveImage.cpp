// Copyright Rive, Inc. All rights reserved.


#include "Slate/SRiveImage.h"
#include "RiveArtboard.h"
#include "RiveWidgetHelpers.h"
#include "Widgets/Images/SImage.h"
#include "SlateOptMacros.h"
#include "Rive/RiveTexture.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

namespace UE::Private::SRiveImage
{
	FVector2f GetInputCoordinates(URiveTexture* InRiveTexture, URiveArtboard* InRiveArtboard, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		// Convert absolute input position to viewport local position
		FDeprecateSlateVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

		// Because our RiveTexture can be a different pixel size than our viewport, we have to scale the x,y coords 
		const FVector2f ViewportSize = MyGeometry.GetLocalSize();
		const FBox2f TextureBox = RiveWidgetHelpers::CalculateRenderTextureExtentsInViewport(InRiveTexture->Size, ViewportSize);
		FRiveArtboardLocalCoordinateData CoordinateData = InRiveTexture->GetLocalCoordinateDataFromExtents(InRiveArtboard, LocalPosition, TextureBox);
		return CoordinateData.GetCalculatedCoordinate();
	}
}

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
	OnInput(MyGeometry, MouseEvent, [this](const FVector2f& InputCoordinates, StateMachinePtr InStateMachine)
	{
		if (InStateMachine)
		{
			InStateMachine->OnMouseMove(InputCoordinates);
		}
	});
	
	return SCompoundWidget::OnMouseMove(MyGeometry, MouseEvent);
}

FReply SRiveImage::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnInput(MyGeometry, MouseEvent, [this](const FVector2f& InputCoordinates, StateMachinePtr InStateMachine)
	{
		if (InStateMachine)
		{
			InStateMachine->OnMouseButtonDown(InputCoordinates);
		}
	});
	
	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SRiveImage::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	OnInput(MyGeometry, MouseEvent, [this](const FVector2f& InputCoordinates, StateMachinePtr InStateMachine)
	{
		if (InStateMachine)
		{
			InStateMachine->OnMouseButtonUp(InputCoordinates);
		}
	});
	
	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SRiveImage::OnTouchMoved(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	OnInput(MyGeometry, InTouchEvent, [this](const FVector2f& InputCoordinates, StateMachinePtr InStateMachine)
	{
		if (InStateMachine)
		{
			InStateMachine->OnMouseMove(InputCoordinates);
		}
	});
	
	return SCompoundWidget::OnTouchMoved(MyGeometry, InTouchEvent);
}

FReply SRiveImage::OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	OnInput(MyGeometry, InTouchEvent, [this](const FVector2f& InputCoordinates, StateMachinePtr InStateMachine)
	{
		if (InStateMachine)
		{
			InStateMachine->OnMouseButtonDown(InputCoordinates);
		}
	});
	
	return SCompoundWidget::OnTouchStarted(MyGeometry, InTouchEvent);
}

FReply SRiveImage::OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	OnInput(MyGeometry, InTouchEvent, [this](const FVector2f& InputCoordinates, StateMachinePtr InStateMachine)
	{
		if (InStateMachine)
		{
			InStateMachine->OnMouseButtonUp(InputCoordinates);
		}
	});
	
	return SCompoundWidget::OnTouchEnded(MyGeometry, InTouchEvent);
}

void SRiveImage::RegisterArtboardInputs(const TArray<URiveArtboard*> InArtboards)
{
	Artboards = InArtboards;
}

void SRiveImage::OnInput(const FGeometry& MyGeometry, const FPointerEvent& InEvent, const FStateMachineInputCallback& InStateMachineInputCallback)
{
#if WITH_RIVE
	URiveTexture* RiveTexture = Cast<URiveTexture>(RiveBrush.GetResourceObject());
	if (!RiveTexture)
	{
		return;
	}
	
	for (URiveArtboard* Artboard : Artboards)
	{
		if (!ensure(Artboard))
		{
			continue;	
		}
		
		Artboard->BeginInput();
		if (const auto StateMachine = Artboard->GetStateMachine())
		{
			FVector2f InputCoordinates = UE::Private::SRiveImage::GetInputCoordinates(RiveTexture, Artboard, MyGeometry, InEvent);
			
			InStateMachineInputCallback(InputCoordinates, StateMachine);
		}
		Artboard->EndInput();
	}
#endif // WITH_RIVE
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
