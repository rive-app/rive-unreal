// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

namespace UE::Rive::Core
{
	class FURStateMachine;
}

class URiveArtboard;
class URiveTexture;
/**
 * 
 */
class RIVE_API SRiveImage : public SCompoundWidget
{
public:
	using StateMachinePtr = UE::Rive::Core::FURStateMachine*;
	using FStateMachineInputCallback = TFunction<void(const FVector2f&, StateMachinePtr)>;
	
	SLATE_BEGIN_ARGS(SRiveImage)
		{
		}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, URiveTexture* InRiveTexture = nullptr);

	void SetRiveTexture(URiveTexture* InRiveTexture);

	// Begin SWidget overrides
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnTouchMoved(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent) override;
	virtual FReply OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent) override;
	virtual FReply OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent) override;

	// End SWidget overrides

	void RegisterArtboardInputs(const TArray<URiveArtboard*> InArtboards);
private:
	void OnInput(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, const FStateMachineInputCallback& InStateMachineInputCallback);
	
private:
	TArray<URiveArtboard*> Artboards;
	FSlateBrush RiveBrush;
};
