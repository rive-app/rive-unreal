// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Components/WidgetComponent.h"
#include "Math/Color.h"
#include "Templates/SharedPointerFwd.h"
#include "UObject/WeakObjectPtrFwd.h"
#include "RiveFullScreenUserWidget_PostProcessBase.generated.h"

class FSceneViewport;
class FWidgetRenderer;
class FRiveWidgetPostProcessHitTester;
class SVirtualWindow;
class SViewport;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextureRenderTarget2D;
class UUserWidget;
class UWorld;

namespace UE::RiveUtilities::Private
{
	class FRiveWidgetPostProcessHitTester;
}

/**
 * Implements shared widget initialization logic.
 */
USTRUCT()
struct FRiveFullScreenUserWidget_PostProcessBase
{
	GENERATED_BODY()
	
	/**
	 * Structor(s)
	 */

public:

	FRiveFullScreenUserWidget_PostProcessBase();

	virtual ~FRiveFullScreenUserWidget_PostProcessBase() = default;

	/**
	 * Implementation(s)
	 */

public:

	/** Deinits this output type */
	virtual void Hide(UWorld* World);

	TSharedPtr<SVirtualWindow> RIVE_API GetSlateWindow() const;

protected:

	bool CreateRenderer(UWorld* World, UUserWidget* Widget, TAttribute<float> InDPIScale);

	virtual void ReleaseRenderer();

	void TickRenderer(UWorld* World, float DeltaSeconds);

private:

	/** Creates the post process material and sets up its parameters. */
	virtual bool OnRenderTargetInited()
	{
		return true;
	};

	/** Determines widget size depending on the viewport type (PIE / Game) */
	FIntPoint CalculateWidgetDrawSize(UWorld* World);

	bool IsTextureSizeValid(FIntPoint Size) const;

	/** Starts detecting input to viewport and relays it to the user widget. */
	void RegisterHitTesterWithViewport(UWorld* World);

	void UnRegisterHitTesterWithViewport();

	/** Gets the viewport to register the hit tester with. */
	TSharedPtr<SViewport> GetViewport(UWorld* World) const;

	/** Determines the DPI to apply for clicks depending on the type of viewport (PIE / Game). */
	float GetDPIScaleForPostProcessHitTester(TWeakObjectPtr<UWorld> World) const;

	/**
	 * Attribute(s)
	 */

public:

	/**
	 * Post process material used to display the widget.
	 * SlateUI [Texture]
	 * TintColorAndOpacity [Vector]
	 * OpacityFromTexture [Scalar]
	 */
	UPROPERTY(EditAnywhere, Category = PostProcess)
	TObjectPtr<UMaterialInterface> PostProcessMaterial;

	/** Tint color and opacity for this component. */
	UPROPERTY(EditAnywhere, Category = PostProcess)
	FLinearColor PostProcessTintColorAndOpacity;

	/** Sets the amount of opacity from the widget's UI texture to use when rendering the translucent or masked UI to the viewport (0.0-1.0). */
	UPROPERTY(EditAnywhere, Category = PostProcess, meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float PostProcessOpacityFromTexture;
	
	/** Whether to overwrite the size of the rendered widget. */
	UPROPERTY(EditAnywhere, Category = PostProcess, meta=(InlineEditConditionToggle))
	bool bUseWidgetDrawSize;
	
	/** The size of the rendered widget. */
	UPROPERTY(EditAnywhere, Category = PostProcess, meta=(EditCondition= bWidgetDrawSize))
	FIntPoint WidgetDrawSize;
	
	/** Is the virtual window created to host the widget focusable? */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PostProcess)
	bool bWindowFocusable;

	/** The visibility of the virtual window created to host the widget. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PostProcess)
	EWindowVisibility WindowVisibility;

	/** Register with the viewport for hardware input from the mouse and keyboard. It can and will steal focus from the viewport. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category= PostProcess)
	bool bReceiveHardwareInput;

	/** The background color of the render target */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PostProcess)
	FLinearColor RenderTargetBackgroundColor;

	/** The blend mode for the widget. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PostProcess)
	EWidgetBlendMode RenderTargetBlendMode;

	/** The target to which the user widget is rendered. */
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> WidgetRenderTarget;

#if WITH_EDITOR

	/**
	 * The viewport to use for displaying.
	 * 
	 * Defaults to GetFirstActiveLevelViewport().
	 */
	TWeakPtr<FSceneViewport> EditorTargetViewport;

#endif // WITH_EDITOR

private:
	
	/** Helper class for drawing widgets to a render target. */
	FWidgetRenderer* WidgetRenderer;
	
	/** The size of the rendered widget */
	FIntPoint CurrentWidgetDrawSize;
	
	/** The slate viewport we are registered to. */
	TWeakPtr<SViewport> ViewportWidget;
	/** The slate window that contains the user widget content. */
	TSharedPtr<SVirtualWindow> SlateWindow;
	
	/** Hit tester when we want the hardware input. */
	TSharedPtr<UE::RiveUtilities::Private::FRiveWidgetPostProcessHitTester> CustomHitTestPath;
};
