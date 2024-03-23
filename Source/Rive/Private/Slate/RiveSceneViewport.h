// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Slate/SceneViewport.h"

class FRiveViewportClient;
class URiveTexture;
class URiveArtboard;

namespace UE::Rive::Core
{
	class FURStateMachine;
}

/**
 * 
 */
class FRiveSceneViewport : public FSceneViewport
{
	/**
	 * Structor(s)
	 */

public:

	FRiveSceneViewport(FRiveViewportClient* InViewportClient, TSharedPtr<SViewport> InViewportWidget, URiveTexture* InRiveTexture, const TArray<URiveArtboard*> InArtboards);
	
	FRiveViewportClient* GetRiveViewportClient() const { return RiveViewportClient; }
	
	//~ BEGIN : FSceneViewport Interface 
public:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	//~ END : FSceneViewport Interface
	
	/**
	 * Attribute(s)
	 */

	void SetRiveTexture(URiveTexture* InRiveTexture);
	void RegisterArtboardInputs(const TArray<URiveArtboard*>& InArtboards);

protected:
	FRiveViewportClient* RiveViewportClient;

private:
	using StateMachinePtr = UE::Rive::Core::FURStateMachine*;
	using FStateMachineInputCallback = TFunction<void(const FVector2f&, StateMachinePtr)>;

	FReply OnInput(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, const FStateMachineInputCallback& InStateMachineInputCallback);

	TObjectPtr<URiveTexture> RiveTexture;
	TArray<URiveArtboard*> Artboards;
	FVector2f LastMousePosition = FVector2f::ZeroVector;
};
