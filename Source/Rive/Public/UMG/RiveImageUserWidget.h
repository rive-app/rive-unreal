// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveArtboard.h"
#include "Blueprint/UserWidget.h"
#include "RiveImageUserWidget.generated.h"

class URiveTexture;
class SRiveImage;

/**
 * 
 */
UCLASS(editinlinenew, BlueprintType, Blueprintable)
class RIVE_API URiveImageUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Rive)
	void SetRiveTexture(URiveTexture* InRiveTexture);

	UFUNCTION(BlueprintCallable, Category = Rive)
	void RegisterArtboardInputs(const TArray<URiveArtboard*> InArtboards);
	
	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	//~ End UWidget Interface

	//~ Begin UVisual Interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End UVisual Interface

#if WITH_EDITOR
	//~ Begin UWidget Interface
	virtual const FText GetPaletteCategory() override;
	//~ End UWidget Interface
#endif

protected:
	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget Interface

private:
	TSharedPtr<SRiveImage> RiveImage;
	TArray<URiveArtboard*> Artboards;
};
