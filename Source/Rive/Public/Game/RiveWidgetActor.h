// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Rive/RiveAudioEngine.h"
#include "GameFramework/Actor.h"
#include "RiveWidgetActor.generated.h"

class UUserWidget;
class URiveWidget;
struct FRiveDescriptor;
class URiveTextureObject;

UCLASS()
class RIVE_API ARiveWidgetActor : public AActor
{
    GENERATED_BODY()

    /**
     * Structor(s)
     */

public:
    ARiveWidgetActor();

    //~ BEGIN : AActor Interface

public:
    virtual void BeginPlay() override;

    //~ END : AActor Interface

    /**
     * Attribute(s)
     */

public:
    void SetWidgetClass(TSubclassOf<UUserWidget> InUserWidget)
    {
        RiveWidgetClass = InUserWidget;
    }
    TSubclassOf<UUserWidget> GetWidgetClass() { return RiveWidgetClass; }

protected:
    /** Settings for Rive Rendering */
    UPROPERTY(EditAnywhere, Category = "Rive", meta = (ShowOnlyInnerProperties))
    TSubclassOf<UUserWidget> RiveWidgetClass;

    /** Settings for Rive Rendering */
    UPROPERTY(VisibleAnywhere,
              Instanced,
              NoClear,
              Category = "User Interface",
              meta = (ShowOnlyInnerProperties))
    TObjectPtr<UUserWidget> ScreenUserWidget;

    UPROPERTY(VisibleAnywhere,
              Instanced,
              NoClear,
              Category = "Audio",
              meta = (ShowOnlyInnerProperties))
    TObjectPtr<URiveAudioEngine> AudioEngine;

    TObjectPtr<URiveWidget> RiveWidget;

    UFUNCTION()
    void OnRiveWidgetReady();
};
