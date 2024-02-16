// Copyright Rive, Inc. All rights reserved.

#include "RiveSceneViewport.h"

#include "RiveViewportClient.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveArtboard.h"
#include "Rive/Core/URStateMachine.h"

FRiveSceneViewport::FRiveSceneViewport(FRiveViewportClient* InViewportClient, TSharedPtr<SViewport> InViewportWidget, URiveFile* InRiveFile)
	: FSceneViewport(InViewportClient, InViewportWidget)
	, RiveViewportClient(InViewportClient), RiveFile(InRiveFile)
{
}

FRiveSceneViewport::~FRiveSceneViewport()
{
}

FReply FRiveSceneViewport::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!RiveFile)
	{
		UE_LOG(LogRive, Warning, TEXT("Could not process FRiveSceneViewport::OnMouseButtonDown as we have an empty rive file."));

		return FReply::Unhandled();
	}

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	// Start a new reply state
	FReply NewReplyState = FSceneViewport::OnMouseButtonDown(MyGeometry, MouseEvent);

#if WITH_RIVE

	RiveFile->BeginInput();

	if (const URiveArtboard* Artboard = RiveFile->GetArtboard())
	{
		if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
		{
			const FVector2f LocalPosition = RiveViewportClient->CalculateLocalPointerCoordinatesFromViewport(MyGeometry, MouseEvent);

			if (StateMachine->OnMouseButtonDown(LocalPosition))
			{
				UE_LOG(LogRive, Warning, TEXT("Handled FRiveSceneViewport::OnMouseButtonDown at '%s'"), *LocalPosition.ToString())
			}
		}
	}

	RiveFile->EndInput();

#endif // WITH_RIVE

	return NewReplyState;
}

FReply FRiveSceneViewport::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!RiveFile)
	{
		UE_LOG(LogRive, Warning, TEXT("Could not process FRiveSceneViewport::OnMouseButtonUp as we have an empty rive file."));

		return FReply::Unhandled();
	}

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	// Start a new reply state
	FReply NewReplyState = FSceneViewport::OnMouseButtonUp(MyGeometry, MouseEvent);

#if WITH_RIVE

	RiveFile->BeginInput();

	if (const URiveArtboard* Artboard = RiveFile->GetArtboard())
	{
		if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
		{
			const FVector2f LocalPosition = RiveViewportClient->CalculateLocalPointerCoordinatesFromViewport(MyGeometry, MouseEvent);

			if (StateMachine->OnMouseButtonUp(LocalPosition))
			{
				UE_LOG(LogRive, Warning, TEXT("Handled FRiveSceneViewport::OnMouseButtonUp at '%s'"), *LocalPosition.ToString())
			}
		}
	}

	RiveFile->EndInput();

#endif // WITH_RIVE
	
	return NewReplyState;
}

FReply FRiveSceneViewport::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!RiveFile)
	{
		UE_LOG(LogRive, Warning, TEXT("Could not process FRiveSceneViewport::OnMouseMove as we have an empty rive file."));

		return FReply::Unhandled();
	}

	if (MouseEvent.GetScreenSpacePosition() == LastMousePosition)
	{
		return FReply::Unhandled();
	}

	// Start a new reply state
	FReply NewReplyState = FSceneViewport::OnMouseMove(MyGeometry, MouseEvent);

#if WITH_RIVE

	RiveFile->BeginInput();

	if (const URiveArtboard* Artboard = RiveFile->GetArtboard())
	{
		if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
		{
			const FVector2f LocalPosition = RiveViewportClient->CalculateLocalPointerCoordinatesFromViewport(MyGeometry, MouseEvent);
			
			if (StateMachine->OnMouseMove(LocalPosition))
			{
				LastMousePosition = MouseEvent.GetScreenSpacePosition();
			}
		}
	}

	RiveFile->EndInput();

#endif // WITH_RIVE
	
	return NewReplyState;
}
