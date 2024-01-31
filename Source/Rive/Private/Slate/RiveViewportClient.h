// Copyright Rive, Inc. All rights reserved.

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
	/**
	 * Structor(s)
	 */

public:
	
	FRiveViewportClient(URiveFile* InRiveFile, const TSharedRef<SRiveWidgetView>& InWidgetView);
	
	~FRiveViewportClient();

	//~ BEGIN : FViewportClient Interface

public:

	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
	
	virtual UWorld* GetWorld() const override { return nullptr; }

	//~ END : FViewportClient Interface

	/**
	 * Attribute(s)
	 */

private:

	TObjectPtr<URiveFile> RiveFile;

	/** Weak Ptr to view widget */
	TWeakPtr<SRiveWidgetView> WidgetViewWeakPtr;
};
