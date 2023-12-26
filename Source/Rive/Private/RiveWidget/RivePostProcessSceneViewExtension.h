// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SceneViewExtension.h"
#include "UObject/WeakObjectPtrFwd.h"

class UMaterialInterface;
class UTextureRenderTarget2D;


/**
 * Renders a post process material over the entirety of the screen.
 * 
 * This method differs from adding a post process material as blend material to a cine camera by
 * ignoring anamorphic squeeze. If the Anamorphic Squeeze == 1, both methods produce the same output; if anamorphic
 * squeeze != 1, the cine camera will smallen the viewport vertically (adding black bars to the top and bottom) and render the blend material.
 * This method will render the post process material over the entire area of the viewport (over the black bars as well).
 *
 * Intended to be used with a post process material that renders a widget over the viewport, e.g. useful for pixel streaming.
 */
class FRivePostProcessSceneViewExtension : public FSceneViewExtensionBase
{
	using Super = FSceneViewExtensionBase;
public:
	
	FRivePostProcessSceneViewExtension(const FAutoRegister& AutoRegister, UTextureRenderTarget2D& WidgetRenderTarget);

	//~ISceneViewExtension interface
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;
	virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;
	//~ISceneViewExtension interface

private:
	
	/** Contains the widget that is supposed to be overlaid. */
	TWeakObjectPtr<UTextureRenderTarget2D> WidgetRenderTarget;
	
	void RenderMaterial_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, FRDGTextureRef ViewFamilyTexture);
};
