// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Slate/SceneViewport.h"

class URiveFile;

/**
 * 
 */
class FRiveSceneViewport : public FSceneViewport
{
	/**
	 * Structor(s)
	 */

public:

	FRiveSceneViewport(FViewportClient* InViewportClient, TSharedPtr<SViewport> InViewportWidget, URiveFile* InRiveFile);
	
	virtual ~FRiveSceneViewport() override;

	//~ BEGIN : FSceneViewport Interface 
	
public:

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	//~ END : FSceneViewport Interface
	
	/**
	 * Attribute(s)
	 */

private:

	TObjectPtr<URiveFile> RiveFile;

	FVector2f LastMousePosition = FVector2f::ZeroVector;
};
