// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RiveFullScreenUserWidget.generated.h"

// Engine\Plugins\Experimental\VirtualProductionUtilities\Source\VPUtilities\Public\Widgets\VPFullScreenUserWidget.h

class FSceneViewport;
class SLevelViewport;
class SConstraintCanvas;
class ARiveActor;

UENUM(BlueprintType)
enum class ERiveWidgetDisplayType : uint8
{
	/** Do not display. */
	Inactive,
	/** Display on a game viewport. */
	Viewport,
	/** Display by adding post process material via post process volume settings. Widget added only over area rendered by anamorphic squeeze. */
	PostProcessWithBlendMaterial,
	/** Render to a texture and send to composure. */
	Composure,
	/** Display by adding post process material via SceneViewExtensions. Widget added over entire viewport ignoring anamorphic squeeze.  */
	PostProcessSceneViewExtension,
};

USTRUCT()
struct FRiveFullScreenUserWidget_Viewport
{
	GENERATED_BODY()

	bool Display(UWorld* World, const TSharedPtr<SWidget>& InWidget, TAttribute<float> InDPIScale);
	void Hide(UWorld* World);

#if WITH_EDITOR
	/**
	 * The viewport to use for displaying.
	 * Defaults to GetFirstActiveLevelViewport().
	 */
	TWeakPtr<FSceneViewport> EditorTargetViewport;
#endif

private:

	/** Constraint widget that contains the widget we want to display. */
	TWeakPtr<SConstraintCanvas> FullScreenCanvasWidget;

#if WITH_EDITOR
	/** Level viewport the widget was added to. */
	TWeakPtr<SLevelViewport> OverlayWidgetLevelViewport;
#endif
};

/**
 * 
 */
UCLASS(meta=(ShowOnlyInnerProperties))
class RIVE_API URiveFullScreenUserWidget : public UObject
{
	GENERATED_BODY()

public:

	URiveFullScreenUserWidget();

	//~ Begin UObject interface
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	virtual bool Display(UWorld* World);
	virtual void Hide();
	virtual void Tick(float DeltaTime);

	/** Sets the way the widget is supposed to be displayed */
	void SetDisplayTypes(ERiveWidgetDisplayType InEditorDisplayType, ERiveWidgetDisplayType InGameDisplayType, ERiveWidgetDisplayType InPIEDisplayType);

	void SetRiveActor(ARiveActor* InActor);
	
	/**
	 * If using ERiveWidgetDisplayType::PostProcessWithBlendMaterial, you can specify a custom post process settings that should be modified.
	 * By default, a new post process component is added to AWorldSettings.
	 *
	 * @param InCustomPostProcessSettingsSource An object containing a FPostProcessSettings UPROPERTY()
	 */
	void SetCustomPostProcessSettingsSource(TWeakObjectPtr<UObject> InCustomPostProcessSettingsSource);
	
	bool ShouldDisplay(UWorld* World) const;
	ERiveWidgetDisplayType GetDisplayType(UWorld* World) const;
	bool IsDisplayed() const;
	
	/** Get a pointer to the inner widget. Note: This should not be stored! */
	UUserWidget* GetWidget() const { return Widget; };

	/** Gets the widget class being displayed */
	TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }
	/** Sets the widget class to be displayed. This must be called while not widget is being displayed - the class will not be updated while IsDisplayed().  */
	void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

	constexpr static bool DoesDisplayTypeUsePostProcessSettings(ERiveWidgetDisplayType Type) { return Type == ERiveWidgetDisplayType::Composure || Type == ERiveWidgetDisplayType::PostProcessSceneViewExtension || Type == ERiveWidgetDisplayType::PostProcessWithBlendMaterial; }
	// /** Returns the base post process settings for the given type if DoesDisplayTypeUsePostProcessSettings(Type) returns true. */
	// FVPFullScreenUserWidget_PostProcessBase* GetPostProcessDisplayTypeSettingsFor(ERiveWidgetDisplayType Type);
	// FVPFullScreenUserWidget_PostProcess& GetPostProcessDisplayTypeWithBlendMaterialSettings() { return PostProcessDisplayTypeWithBlendMaterial; }
	// FVPFullScreenUserWidget_PostProcess& GetComposureDisplayTypeSettings() { return PostProcessDisplayTypeWithBlendMaterial; }
	// FVPFullScreenUserWidget_PostProcessWithSVE& GetPostProcessDisplayTypeWithSceneViewExtensionsSettings() { return PostProcessWithSceneViewExtensions; }
	// /** Returns the base post process settings for the given type if DoesDisplayTypeUsePostProcessSettings(Type) returns true. */
	// const FVPFullScreenUserWidget_PostProcessBase* GetPostProcessDisplayTypeSettingsFor(ERiveWidgetDisplayType Type) const;
	// const FVPFullScreenUserWidget_PostProcess& GetPostProcessDisplayTypeWithBlendMaterialSettings() const { return PostProcessDisplayTypeWithBlendMaterial; }
	// const FVPFullScreenUserWidget_PostProcess& GetComposureDisplayTypeSettings() const { return PostProcessDisplayTypeWithBlendMaterial; }
	// const FVPFullScreenUserWidget_PostProcessWithSVE& GetPostProcessDisplayTypeWithSceneViewExtensionsSettings() const { return PostProcessWithSceneViewExtensions; }

#if WITH_EDITOR
	/**
	 * Sets the TargetViewport to use on both the Viewport and the PostProcess class.
	 * 
	 * Overrides the viewport to use for displaying.
	 * Defaults to GetFirstActiveLevelViewport().
	 */
	void SetEditorTargetViewport(TWeakPtr<FSceneViewport> InTargetViewport);
	/** Resets the TargetViewport  */
	void ResetEditorTargetViewport();
#endif

protected:
	
	/** The class of User Widget to create and display an instance of */
	UPROPERTY(EditAnywhere, Category = "User Interface")
	TSubclassOf<UUserWidget> WidgetClass;
	
	/** The display type when the world is an editor world. */
	UPROPERTY(EditAnywhere, Category = "User Interface")
	ERiveWidgetDisplayType EditorDisplayType;

	/** The display type when the world is a game world. */
	UPROPERTY(EditAnywhere, Category = "User Interface")
	ERiveWidgetDisplayType GameDisplayType;

	/** The display type when the world is a PIE world. */
	UPROPERTY(EditAnywhere, Category = "User Interface", meta = (DisplayName = "PIE Display Type"))
	ERiveWidgetDisplayType PIEDisplayType;

	/** Behavior when the widget should be display by the slate attached to the viewport. */
	UPROPERTY(EditAnywhere, Category = "Viewport", meta = (ShowOnlyInnerProperties))
	FRiveFullScreenUserWidget_Viewport ViewportDisplayType;

	// /** Behavior when the widget is rendered via a post process material via post process volume settings; the widget is added only over area rendered by anamorphic squeeze. */
	// UPROPERTY(EditAnywhere, Category = "Post Process (Blend Material)")
	// FVPFullScreenUserWidget_PostProcess PostProcessDisplayTypeWithBlendMaterial;
	//
	// /** Behavior when the widget is rendered via a post process material via SceneViewExtensions; the widget is added over entire viewport, ignoring anamorphic squeeze. */
	// UPROPERTY(EditAnywhere, Category = "Post Process (Scene View Extensions)")
	// FVPFullScreenUserWidget_PostProcessWithSVE PostProcessWithSceneViewExtensions;
	//
	bool InitWidget();
	void ReleaseWidget();

	FVector2D FindSceneViewportSize();
	float GetViewportDPIScale();

private:
	
	/** The User Widget object displayed and managed by this component */
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UUserWidget> Widget;

	UPROPERTY()
	TObjectPtr<ARiveActor> RiveActor;

	/** The world the widget is attached to. */
	UPROPERTY(Transient, DuplicateTransient)
	TWeakObjectPtr<UWorld> World;

	/** How we currently displaying the widget. */
	UPROPERTY(Transient, DuplicateTransient)
	ERiveWidgetDisplayType CurrentDisplayType;

	/** The user requested the widget to be displayed. It's possible that some setting are invalid and the widget will not be displayed. */
	bool bDisplayRequested;

#if WITH_EDITOR
	/**
	 * The viewport to use for displaying.
	 * Defaults to GetFirstActiveLevelViewport().
	 */
	TWeakPtr<FSceneViewport> EditorTargetViewport;
#endif
	
	void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld);
	void OnWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources);
};
