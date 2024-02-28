// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "RiveImage.generated.h"


class URiveImageUserWidget;
class URiveArtboard;
class SRiveImage;

/**
 * 
 */
UCLASS()
class RIVE_API URiveImage : public UWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Rive)
	void SetRiveTexture(URiveTexture* InRiveTexture);

	UFUNCTION(BlueprintCallable, Category = Rive)
	void RegisterArtboardInputs(const TArray<URiveArtboard*> InArtboards);
	
	UFUNCTION(BlueprintCallable, Category = Rive)
	void Setup(URiveTexture* InRiveTexture, const TArray<URiveArtboard*> InArtboards);

	void SetUserWidget(URiveImageUserWidget* InUserWidget);

	//~ Begin UVisual Interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End UVisual Interface
	
#if WITH_EDITOR
	//~ Begin UWidget Interface
	virtual const FText GetPaletteCategory() override;
	//~ End UWidget Interface
#endif

	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget Interface

private:
	UPROPERTY()
	TObjectPtr<URiveImageUserWidget> UserWidget;

	TSharedPtr<SRiveImage> RiveImage;

	TArray<TObjectPtr<URiveArtboard>> Artboards;
};
