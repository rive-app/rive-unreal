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

		// We ask our artboard delegate (if there is one) to provide us with the translation offset it could be rendering in
		// This is useful when artboards implement their own blueprint render code, and are dynamically shifting the position of the artboard inside the render target
		FVector2f ArtboardOffset;
		if (InRiveArtboard->OnGetLocalCoordinates.IsBound())
		{
			ArtboardOffset = InRiveArtboard->OnGetLocalCoordinates.Execute({0, 0});
		}

		float InputX = LocalPosition.X / ScaleFactorWidth - ArtboardOffset.X;
		float InputY = LocalPosition.Y / ScaleFactorHeight - ArtboardOffset.Y;
		return {
			static_cast<float>(FMath::Clamp(InputX, 0.0, InputX)),
			static_cast<float>(FMath::Clamp(InputY, 0.0, InputY)),
		};
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
	UE_LOG(LogTemp, Warning, TEXT("OnMouseMove %s"), *MouseEvent.GetScreenSpacePosition().ToString());

#if WITH_RIVE
	URiveTexture* RiveTexture = Cast<URiveTexture>(RiveBrush.GetResourceObject());
	for (URiveArtboard* Artboard : Artboards)
	{
		Artboard->BeginInput();
		if (auto StateMachine = Artboard->GetStateMachine())
		{
			FVector2f InputCoordinates = UE::Private::SRiveImage::GetInputCoordinates(RiveTexture, Artboard, MyGeometry, MouseEvent);
			StateMachine->OnMouseMove(InputCoordinates);
		}
		Artboard->EndInput();
	}
#endif // WITH_RIVE
	return SCompoundWidget::OnMouseMove(MyGeometry, MouseEvent);
}

FReply SRiveImage::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnMouseButtonDown %s"), *MouseEvent.GetScreenSpacePosition().ToString());

#if WITH_RIVE
	URiveTexture* RiveTexture = Cast<URiveTexture>(RiveBrush.GetResourceObject());
	for (URiveArtboard* Artboard : Artboards)
	{
		Artboard->BeginInput();
		if (auto StateMachine = Artboard->GetStateMachine())
		{
			FVector2f InputCoordinates = UE::Private::SRiveImage::GetInputCoordinates(RiveTexture, Artboard, MyGeometry, MouseEvent);
			StateMachine->OnMouseButtonDown(InputCoordinates);
		}
		Artboard->EndInput();
	}
#endif // WITH_RIVE
	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SRiveImage::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnMouseButtonUp %s"), *MouseEvent.GetScreenSpacePosition().ToString());

#if WITH_RIVE
	URiveTexture* RiveTexture = Cast<URiveTexture>(RiveBrush.GetResourceObject());
	for (URiveArtboard* Artboard : Artboards)
	{
		Artboard->BeginInput();
		if (auto StateMachine = Artboard->GetStateMachine())
		{
			FVector2f InputCoordinates = UE::Private::SRiveImage::GetInputCoordinates(RiveTexture, Artboard, MyGeometry, MouseEvent);
			StateMachine->OnMouseButtonUp(InputCoordinates);
		}
		Artboard->EndInput();
	}
#endif // WITH_RIVE
	
	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SRiveImage::OnTouchMoved(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnTouchMoved %s"), *InTouchEvent.GetScreenSpacePosition().ToString());

#if WITH_RIVE
	URiveTexture* RiveTexture = Cast<URiveTexture>(RiveBrush.GetResourceObject());
	for (URiveArtboard* Artboard : Artboards)
	{
		Artboard->BeginInput();
		if (auto StateMachine = Artboard->GetStateMachine())
		{
			FVector2f InputCoordinates = UE::Private::SRiveImage::GetInputCoordinates(RiveTexture, Artboard, MyGeometry, InTouchEvent);
			StateMachine->OnMouseMove(InputCoordinates);
		}
		Artboard->EndInput();
	}
#endif // WITH_RIVE
	
	return SCompoundWidget::OnTouchMoved(MyGeometry, InTouchEvent);
}

FReply SRiveImage::OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnTouchStarted %s"), *InTouchEvent.GetScreenSpacePosition().ToString());

#if WITH_RIVE
	URiveTexture* RiveTexture = Cast<URiveTexture>(RiveBrush.GetResourceObject());
	for (URiveArtboard* Artboard : Artboards)
	{
		Artboard->BeginInput();
		if (auto StateMachine = Artboard->GetStateMachine())
		{
			FVector2f InputCoordinates = UE::Private::SRiveImage::GetInputCoordinates(RiveTexture, Artboard, MyGeometry, InTouchEvent);
			StateMachine->OnMouseButtonDown(InputCoordinates);
		}
		Artboard->EndInput();
	}
#endif // WITH_RIVE
	
	return SCompoundWidget::OnTouchStarted(MyGeometry, InTouchEvent);
}

FReply SRiveImage::OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	UE_LOG(LogTemp, Warning, TEXT("OnTouchEnded %s"), *InTouchEvent.GetScreenSpacePosition().ToString());

#if WITH_RIVE
	URiveTexture* RiveTexture = Cast<URiveTexture>(RiveBrush.GetResourceObject());
	for (URiveArtboard* Artboard : Artboards)
	{
		Artboard->BeginInput();
		if (auto StateMachine = Artboard->GetStateMachine())
		{
			FVector2f InputCoordinates = UE::Private::SRiveImage::GetInputCoordinates(RiveTexture, Artboard, MyGeometry, InTouchEvent);
			StateMachine->OnMouseButtonUp(InputCoordinates);
		}
		Artboard->EndInput();
	}
#endif // WITH_RIVE
	
	return SCompoundWidget::OnTouchEnded(MyGeometry, InTouchEvent);
}

void SRiveImage::RegisterArtboardInputs(const TArray<URiveArtboard*> InArtboards)
{
	Artboards = InArtboards;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
