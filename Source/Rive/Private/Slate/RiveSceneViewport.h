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
public:
	FRiveSceneViewport(FViewportClient* InViewportClient, TSharedPtr<SViewport> InViewportWidget, URiveFile* InRiveFile);
	virtual ~FRiveSceneViewport() override;


	//~ Begin FSceneViewport Interface 
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	//~ End FSceneViewport Interface

private:
	TObjectPtr<URiveFile> RiveFile;

	FVector2f LastMousePosition = FVector2f::ZeroVector;
};
