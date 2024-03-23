// Copyright Rive, Inc. All rights reserved.

#include "RiveSceneViewport.h"

#include "RiveViewportClient.h"
#include "RiveWidgetHelpers.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveTexture.h"
#include "RiveArtboard.h"
#include "RiveCore/Public/RiveArtboard.h"
#include "RiveCore/Public/URStateMachine.h"

#include "RiveWidgetHelpers.h"

namespace UE::Private::FRiveSceneViewport
{
	FVector2f GetInputCoordinates(URiveTexture* InRiveTexture, URiveArtboard* InRiveArtboard, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		// Convert absolute input position to viewport local position
		FDeprecateSlateVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

		// Because our RiveTexture can be a different pixel size than our viewport, we have to scale the x,y coords 
		const FVector2f ViewportSize = MyGeometry.GetLocalSize();
		const FBox2f TextureBox = RiveWidgetHelpers::CalculateRenderTextureExtentsInViewport(InRiveTexture->Size, ViewportSize);
		return InRiveTexture->GetLocalCoordinatesFromExtents(InRiveArtboard, LocalPosition, TextureBox);
	}
}

FRiveSceneViewport::FRiveSceneViewport(FRiveViewportClient* InViewportClient, TSharedPtr<SViewport> InViewportWidget, URiveTexture* InRiveTexture, const TArray<URiveArtboard*> InArtboards)
	: FSceneViewport(InViewportClient, InViewportWidget)
	, RiveViewportClient(InViewportClient), RiveTexture(InRiveTexture), Artboards(InArtboards)
{
}

FReply FRiveSceneViewport::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	
	return OnInput(MyGeometry, MouseEvent, [this](const FVector2f& InputCoordinates, StateMachinePtr InStateMachine)
		{
			if (InStateMachine)
			{
				InStateMachine->OnMouseButtonDown(InputCoordinates);
			}
		});
}

FReply FRiveSceneViewport::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	
	return OnInput(MyGeometry, MouseEvent, [this](const FVector2f& InputCoordinates, StateMachinePtr InStateMachine)
		{
			if (InStateMachine)
			{
				InStateMachine->OnMouseButtonUp(InputCoordinates);
			}
		});
}

FReply FRiveSceneViewport::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return OnInput(MyGeometry, MouseEvent, [this](const FVector2f& InputCoordinates, StateMachinePtr InStateMachine)
		{
			if (InStateMachine)
			{
				InStateMachine->OnMouseMove(InputCoordinates);
			}
		});

}

void FRiveSceneViewport::SetRiveTexture(URiveTexture* InRiveTexture)
{
	RiveTexture = InRiveTexture;
}

void FRiveSceneViewport::RegisterArtboardInputs(const TArray<URiveArtboard*>& InArtboards)
{
	Artboards = InArtboards;
}


FReply FRiveSceneViewport::OnInput(const FGeometry& MyGeometry, const FPointerEvent& InEvent, const FStateMachineInputCallback& InStateMachineInputCallback)
{
#if WITH_RIVE
	if (!IsValid(RiveTexture))
	{
		return FReply::Unhandled();
	}

	for (URiveArtboard* Artboard : Artboards)
	{
		if (!ensure(IsValid(Artboard)))
		{
			continue;
		}

		Artboard->BeginInput();
		if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
		{
			FVector2f InputCoordinates = UE::Private::FRiveSceneViewport::GetInputCoordinates(RiveTexture, Artboard, MyGeometry, InEvent);
			InStateMachineInputCallback(InputCoordinates, StateMachine);
		}
		Artboard->EndInput();
	}
#endif // WITH_RIVE

	return FReply::Handled(); // TODO. Should it be Handled all the time?

}