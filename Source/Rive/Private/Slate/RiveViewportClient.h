// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UObject/GCObject.h"
#include "UnrealClient.h"
#include "ViewportClient.h"

class SRiveWidgetView;
class URiveFile;
/**
 * 
 */
class FRiveViewportClient : public FViewportClient
{
public:
	FRiveViewportClient(URiveFile* InRiveFile, const TSharedRef<SRiveWidgetView>& InWidgetView);
	~FRiveViewportClient();

	/** FViewportClient interface */
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
	virtual UWorld* GetWorld() const override { return nullptr; }
private:
	TObjectPtr<URiveFile> RiveFile;

	/** Weak Ptr to view widget */
	TWeakPtr<SRiveWidgetView> WidgetViewWeakPtr;
};
