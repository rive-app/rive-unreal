// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2DDynamic.h"
#include "RiveTexture.generated.h"

class FRiveTextureResource;

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class RIVE_API URiveTexture : public UTexture2DDynamic
{
	GENERATED_BODY()

public:
	URiveTexture();
	
	//~ BEGIN : UTexture Interface
	virtual FTextureResource* CreateResource() override;
	virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) override;
	//~ END : UTexture UTexture

	//~ BEGIN : UTexture Interface
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
	//~ END : UTexture UTexture

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
	FIntPoint Size;

protected:
	/**
	 * Create Texture Rendering resource on RHI Thread
	 */
	void InitializeResources() const;

	/**
	 * Resize render resources after loading artboard but before start Rive Rendering
	 */
	virtual void ResizeRenderTargets(const FIntPoint InNewSize);
	virtual void ResizeRenderTargets(const FVector2f InNewSize);

protected:

	/**
	 * Rendering resource for Rive File
	 */
	FRiveTextureResource* CurrentResource;
};
