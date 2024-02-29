// Copyright Rive, Inc. All rights reserved.


#include "Slate/SRiveImage.h"
#include "RiveArtboard.h"
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
		float ScaleFactorWidth = ViewportSize.X / (float)InRiveTexture->SizeX;
		float ScaleFactorHeight = ViewportSize.Y / (float)InRiveTexture->SizeY;

		float InputX = LocalPosition.X / ScaleFactorWidth;
		float InputY = LocalPosition.Y / ScaleFactorHeight;
		
		// We ask our artboard delegate (if there is one) to provide us with the translation offset it could be rendering in
		// This is useful when artboards implement their own blueprint render code, and are dynamically shifting the position of the artboard inside the render target
		FVector2f InputVector;
		if (InRiveArtboard->OnGetLocalCoordinate.IsBound())
		{
			InputVector = InRiveArtboard->OnGetLocalCoordinate.Execute(InRiveArtboard, {InputX, InputY});
		} else
		{
			InputVector = InRiveArtboard->GetLocalCoordinate({InputX, InputY}, InRiveTexture->Size, {0,0}, ERiveAlignment::TopLeft, ERiveFitType::None);
		}
		
		return {InputVector.X, InputVector.Y};
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
