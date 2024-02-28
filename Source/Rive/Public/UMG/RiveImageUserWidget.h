// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveArtboard.h"
#include "Blueprint/UserWidget.h"
#include "RiveImageUserWidget.generated.h"

class URiveTexture;
class UCanvasPanel;
class URiveImage;
class UCanvasPanelSlot;
class URiveFile;

/**
 * 
 */
UCLASS(editinlinenew, BlueprintType, Blueprintable)
class RIVE_API URiveImageUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, BlueprintReadOnly, VisibleAnywhere, Category = "Rive|UI")
	TObjectPtr<UCanvasPanel> RootCanvasPanel;

	UPROPERTY(Transient, BlueprintReadOnly, VisibleAnywhere, Category = "Rive|UI")
	TObjectPtr<URiveImage> RiveImage;

	UPROPERTY(Transient, BlueprintReadOnly, VisibleAnywhere, Category = "Rive|UI")
	TObjectPtr<UCanvasPanelSlot> RiveImageSlot;

protected:
	//~ Begin UUserWidget Interface
	virtual bool Initialize() override;
	//~ End UUserWidget Interface
	
	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget Interface

	UFUNCTION(BlueprintCallable, Category = Rive)
	static void CalculateCenterPlacementInViewport(const FVector2f& TextureSize, const FVector2f& InViewportSize, FVector2f& OutPosition, FVector2f& OutSize);
};
