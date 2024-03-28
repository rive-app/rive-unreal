// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UObject/GCObject.h"
#include "UnrealClient.h"
#include "ViewportClient.h"
#include "UObject/ObjectPtr.h"

class SRiveWidgetView;
class URiveTexture;
class URiveArtboard;
class UTexture2D;

/**
 * 
 */
class FRiveViewportClient : public FViewportClient
{
	/**
	 * Structor(s)
	 */
public:
	FRiveViewportClient(URiveTexture* InRiveTexture, const TArray<URiveArtboard*> InArtboards, const TSharedRef<SRiveWidgetView>& InWidgetView);

	virtual ~FRiveViewportClient() override;

	//~ BEGIN : FViewportClient Interface
public:
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
	virtual UWorld* GetWorld() const override { return nullptr; }
	//~ END : FViewportClient Interface

	/**
	 * Attribute(s)
	 */

	void SetRiveTexture(URiveTexture* InRiveTexture);
	void RegisterArtboardInputs(const TArray<URiveArtboard*>& InArtboards);

private:
	TObjectPtr<URiveTexture> RiveTexture;
	/** Weak Ptr to view widget */
	TWeakPtr<SRiveWidgetView> WidgetViewWeakPtr;

#if WITH_EDITOR // Implementation of Checkerboard textures, as per FTextureEditorViewportClient::ModifyCheckerboardTextureColors
public:
	bool bDrawCheckeredTexture = false;
private:
	/** Modifies the checkerboard texture's data */
	void ModifyCheckerboardTextureColors();
	/** Destroy the checkerboard texture if one exists */
	void DestroyCheckerboardTexture();
	/** Checkerboard texture */
	TObjectPtr<UTexture2D> CheckerboardTexture;
#endif
};
