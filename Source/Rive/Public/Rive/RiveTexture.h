// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveArtboard.h"
#include "Engine/Texture2DDynamic.h"
#include "RiveTexture.generated.h"

class URiveArtboard;
class FRiveTextureResource;

#define RIVE_MIN_TEX_RESOLUTION 1
#define RIVE_MAX_TEX_RESOLUTION 3840
#define RIVE_STANDARD_TEX_RESOLUTION 3840

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class RIVE_API URiveTexture : public UTexture2DDynamic
{
	GENERATED_BODY()

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnResourceInitializedOnRenderThread, FRHICommandListImmediate& /*RHICmdList*/, FTextureRHIRef& /*NewResource*/)
	
public:
	URiveTexture();
	
	//~ BEGIN : UTexture Interface
	virtual FTextureResource* CreateResource() override;
	//~ END : UTexture UTexture

	//~ BEGIN : UTexture Interface
	virtual void PostLoad() override;
	//~ END : UTexture UTexture

public:
	/** UI representation of Texture Size */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive, meta = (ClampMin = 1, UIMin = 1, ClampMax = 3840, UIMax = 3840, NoResetToDefault))
	FIntPoint Size;

	/**
	 * Resize render resources
	 */
	UFUNCTION(BlueprintCallable, Category = Rive)
	virtual void ResizeRenderTargets(const FIntPoint InNewSize);
	
	FVector2f GetLocalCoordinatesFromExtents(URiveArtboard* InArtboard, const FVector2f& InPosition, const FBox2f& InExtents) const;

	ESimpleElementBlendMode GetSimpleElementBlendMode() const;

	
	FOnResourceInitializedOnRenderThread OnResourceInitializedOnRenderThread;
protected:
	/**
	 * Create Texture Rendering resource on RHI Thread
	 */
	void InitializeResources() const;

	/**
	 * Resize render resources
	 */
	virtual void ResizeRenderTargets(const FVector2f InNewSize);

protected:

	/**
	 * Rendering resource for Rive File
	 */
	FRiveTextureResource* CurrentResource = nullptr;

	UPROPERTY(EditAnywhere, Category = Rive)
	ERiveBlendMode RiveBlendMode = ERiveBlendMode::SE_BLEND_AlphaComposite;
};
