// Copyright Rive, Inc. All rights reserved.


#include "RiveSceneViewport.h"

#include "Logs/RiveLog.h"
#include "Rive/RiveFile.h"

FRiveSceneViewport::FRiveSceneViewport(FViewportClient* InViewportClient, TSharedPtr<SViewport> InViewportWidget, URiveFile* InRiveFile)
	: FSceneViewport(InViewportClient, InViewportWidget)
	, RiveFile(InRiveFile)
{
}

FRiveSceneViewport::~FRiveSceneViewport()
{
}

FReply FRiveSceneViewport::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!RiveFile)
	{
		UE_LOG(LogRive, Warning, TEXT("Could not process FRiveSlateViewport::OnMouseButtonDown as we have an empty rive file."));

		return FReply::Unhandled();
	}

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

#if WITH_RIVE

	RiveFile->BeginInput();

	if (const UE::Rive::Core::FURArtboard* Artboard = RiveFile->GetArtboard())
	{
		if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
		{
			const FSlateRect BoundingRect = MyGeometry.GetRenderBoundingRect();

			const FBox2f ScreenRect({ BoundingRect.Left, BoundingRect.Top }, { BoundingRect.Right, BoundingRect.Bottom });

			const FVector2f LocalPosition = RiveFile->GetLocalCoordinates(MouseEvent.GetScreenSpacePosition(), ScreenRect);

			if (StateMachine->OnMouseButtonDown(LocalPosition))
			{
				RiveFile->EndInput();

				return FReply::Handled();
			}
		}
	}

	RiveFile->EndInput();

#endif // WITH_RIVE

	return FSceneViewport::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply FRiveSceneViewport::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!RiveFile)
	{
		UE_LOG(LogRive, Warning, TEXT("Could not process FRiveSlateViewport::OnMouseButtonUp as we have an empty rive file."));

		return FReply::Unhandled();
	}

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

#if WITH_RIVE

	RiveFile->BeginInput();

	if (const UE::Rive::Core::FURArtboard* Artboard = RiveFile->GetArtboard())
	{
		if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
		{
			const FSlateRect BoundingRect = MyGeometry.GetRenderBoundingRect();

			const FBox2f ScreenRect({ BoundingRect.Left, BoundingRect.Top }, { BoundingRect.Right, BoundingRect.Bottom });

			const FVector2f LocalPosition = RiveFile->GetLocalCoordinates(MouseEvent.GetScreenSpacePosition(), ScreenRect);

			if (StateMachine->OnMouseButtonUp(LocalPosition))
			{
				RiveFile->EndInput();

				return FReply::Handled();
			}
		}
	}

	RiveFile->EndInput();

#endif // WITH_RIVE
	
	return FSceneViewport::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply FRiveSceneViewport::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!RiveFile)
	{
		UE_LOG(LogRive, Warning, TEXT("Could not process FRiveSlateViewport::OnMouseMove as we have an empty rive file."));

		return FReply::Unhandled();
	}

	if (MouseEvent.GetScreenSpacePosition() == LastMousePosition)
	{
		return FReply::Unhandled();
	}

#if WITH_RIVE

	RiveFile->BeginInput();

	if (const UE::Rive::Core::FURArtboard* Artboard = RiveFile->GetArtboard())
	{
		if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
		{
			const FSlateRect BoundingRect = MyGeometry.GetRenderBoundingRect();

			const FBox2f ScreenRect({ BoundingRect.Left, BoundingRect.Top }, { BoundingRect.Right, BoundingRect.Bottom });

			const FVector2f LocalPosition = RiveFile->GetLocalCoordinates(MouseEvent.GetScreenSpacePosition(), ScreenRect);

			if (StateMachine->OnMouseMove(LocalPosition))
			{
				LastMousePosition = MouseEvent.GetScreenSpacePosition();

				RiveFile->EndInput();

				return FReply::Handled();
			}
		}
	}

	RiveFile->EndInput();

#endif // WITH_RIVE
	
	return FSceneViewport::OnMouseMove(MyGeometry, MouseEvent);
}
