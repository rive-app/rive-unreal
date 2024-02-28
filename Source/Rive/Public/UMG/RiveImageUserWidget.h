// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RiveImageUserWidget.generated.h"

class URiveTexture;
class URiveArtboard;
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
	
	UFUNCTION(BlueprintCallable, Category = Rive)
	void Setup(URiveTexture* InRiveTexture, const TArray<URiveArtboard*> InArtboards);

protected:
	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget Interface

	//~ Begin UVisual Interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End UVisual Interface

private:
	TSharedPtr<SRiveImage> RiveImage;

	TArray<TObjectPtr<URiveArtboard>> Artboards;
};
