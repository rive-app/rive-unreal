// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RiveFullScreenUserWidget_PostProcessBase.h"
#include "RiveFullScreenUserWidget_PostProcess.generated.h"

class ACompositingElement;
class UPostProcessComponent;

/**
 * Renders widget by adding it as a blend material.
 */
USTRUCT()
struct FRiveFullScreenUserWidget_PostProcess : public FRiveFullScreenUserWidget_PostProcessBase
{
	GENERATED_BODY()
	
	FRiveFullScreenUserWidget_PostProcess();

	void SetCustomPostProcessSettingsSource(TWeakObjectPtr<UObject> InCustomPostProcessSettingsSource);
	
	bool Display(UWorld* World, UUserWidget* Widget, bool bInRenderToTextureOnly, TAttribute<float> InDPIScale);
	virtual void Hide(UWorld* World) override;
	void Tick(UWorld* World, float DeltaSeconds);

private:
	
	/** Post process component used to add the material to the post process chain. */
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> PostProcessComponent;
	
	/** The dynamic instance of the material that the render target is attached to. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PostProcessMaterialInstance;

	/** Only render to the UTextureRenderTarget2D - do not output to the final viewport. */
	bool bRenderToTextureOnly;
	
	/**
	 * Optional. Some object that contains a FPostProcessSettings property. These settings will be used for PostProcessMaterialInstance.
	 */
	TWeakObjectPtr<UObject> CustomPostProcessSettingsSource;
	
	bool InitPostProcessComponent(UWorld* World);
	bool UpdateTargetPostProcessSettingsWithMaterial();
	void ReleasePostProcessComponent();
	
	virtual bool OnRenderTargetInited() override;

	FPostProcessSettings* GetPostProcessSettings() const;
};
