// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"
#include "RiveDescriptor.h"
#include "RiveTexture.h"
#include "RiveTypes.h"
#include "Tickable.h"
#include "RiveObject.generated.h"

#if WITH_RIVE

struct FAssetImportInfo;
class URiveArtboard;
class FRiveTextureResource;

namespace rive
{
	class File;
}

#endif // WITH_RIVE

class URiveAsset;
class UUserWidget;
class URiveFile;

/**
 *
 */
UCLASS(BlueprintType, Blueprintable, HideCategories=("ImportSettings", "Compression", "Adjustments", "LevelOfDetail", "Compositing"))
class RIVE_API URiveObject : public URiveTexture, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/**
	 * Structor(s)
	 */
	
	URiveObject();
	
	virtual void BeginDestroy() override;
	
	//~ BEGIN : FTickableGameObject Interface

public:
	virtual TStatId GetStatId() const override;

	virtual void Tick(float InDeltaSeconds) override;

	virtual bool IsTickable() const override;

	virtual bool IsTickableInEditor() const override
	{
		return true;
	}

	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Conditional;
	}

	//~ END : FTickableGameObject Interface

	/**
	 * Implementation(s)
	 */

public:

	UFUNCTION(BlueprintPure, Category = Rive)
	FLinearColor GetClearColor() const;

	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetLocalCoordinate(URiveArtboard* InArtboard, const FVector2f& InPosition);

	/**
	 * Returns the coordinates in the current Artboard space
	 * @param InExtents Extents of the RenderTarget, will be mapped to the RenderTarget size
	 */
	UFUNCTION(BlueprintCallable, Category = Rive)
	FVector2f GetLocalCoordinatesFromExtents(const FVector2f& InPosition, const FBox2f& InExtents) const;
	

	/**
	 * Initialize this Rive file by creating the Render Targets and importing the native Rive File 
	 */
	// void Initialize();
	UFUNCTION(BlueprintCallable, Category=Rive)
	void Initialize(const FRiveDescriptor& InRiveDescriptor);
	// void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

	// TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }

	UFUNCTION(BlueprintCallable, Category = Rive)
	URiveArtboard* GetArtboard() const;
	
protected:

	void OnResourceInitialized_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef& NewResource) const;

public:
	UPROPERTY(BlueprintReadWrite, Category=Rive)
	FRiveDescriptor RiveDescriptor;

private:
	UFUNCTION()
	void OnArtboardTickRender(float DeltaTime, URiveArtboard* InArtboard);
	
	UPROPERTY(EditAnywhere, Category = Rive)
	FLinearColor ClearColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, Category = Rive)
	ERiveFitType RiveFitType = ERiveFitType::Contain;

	/* This property is not editable via Editor in Unity, so we'll hide it also */
	UPROPERTY(EditAnywhere, Category=Rive)
	ERiveAlignment RiveAlignment = ERiveAlignment::Center;

	UPROPERTY(EditAnywhere, Category = Rive)
	bool bIsRendering = true;

	/** Control Size of Render Texture Manually */
	UPROPERTY(EditAnywhere, Category = Rive)
	bool bManualSize = false;


	TSharedPtr<IRiveRenderTarget> RiveRenderTarget;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category=Rive, meta=(NoResetToDefault, AllowPrivateAccess, ShowInnerProperties))
	URiveArtboard* Artboard = nullptr;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category=Rive, meta=(AllowPrivateAccess))
	URiveAudioEngine* AudioEngine = nullptr;
	FDelegateHandle AudioEngineLambdaHandle;

public:

};
