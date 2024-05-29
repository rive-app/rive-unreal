// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "RiveActor.generated.h"

class URiveAudioEngine;
class URiveFile;
class URiveFullScreenUserWidget;

UCLASS()
class RIVE_API ARiveActor : public AActor
{
	GENERATED_BODY()

	/**
	 * Structor(s)
	 */

public:

	ARiveActor();

	//~ BEGIN : AActor Interface

public:

	void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);
	virtual void PostLoad() override;

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	//~ END : AActor Interface

	/**
	 * Attribute(s)
	 */

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rive")
	URiveFile* RiveFile = nullptr;

	/** Settings for Rive Rendering */
	UPROPERTY(VisibleAnywhere, Category = "User Interface", meta = (ShowOnlyInnerProperties))
	TSubclassOf<UUserWidget> UserWidgetClass;
	
protected:

	/** Settings for Rive Rendering */
	UPROPERTY(VisibleAnywhere, Instanced, NoClear, Category = "User Interface", meta = (ShowOnlyInnerProperties))
	UUserWidget* ScreenUserWidget = nullptr;

	UPROPERTY(VisibleAnywhere, Instanced, NoClear, Category = "Audio", meta = (ShowOnlyInnerProperties))
	URiveAudioEngine* AudioEngine = nullptr;

#if WITH_EDITORONLY_DATA
	
	/** Display requested and will be executed on the first frame because we can't call BP function in the loading phase */
	bool bEditorDisplayRequested;

#endif // WITH_EDITORONLY_DATA
};
