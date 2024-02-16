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

	virtual ~FRiveViewportClient() override;

	//~ BEGIN : FViewportClient Interface

public:

	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;

	FVector2f CalculateLocalPointerCoordinatesFromViewport(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;
	/**
	 * Calculates the extents of the RiveFIle RenderTarget to be 'Contained' within the given viewport
	 */
	FBox2f CalculateRenderTextureExtentsInViewport(const FVector2f& InViewportSize) const;
	
	virtual UWorld* GetWorld() const override { return nullptr; }

	//~ END : FViewportClient Interface

	/**
	 * Attribute(s)
	 */

private:

	TObjectPtr<URiveFile> RiveFile;

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
