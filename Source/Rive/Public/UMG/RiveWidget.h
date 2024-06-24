// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "rive/file.hpp"
#include "Rive/RiveDescriptor.h"
#include "Rive/RiveFile.h"
#include "RiveWidget.generated.h"

class URiveObject;
class URiveArtboard;
class URiveTexture;
class SRiveWidget;
class URiveAudioEngine;
class URiveFile;

/**
 *
 */
UCLASS()
class RIVE_API URiveWidget : public UUserWidget
{
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRiveReadyDelegate);
    
    GENERATED_BODY()

    virtual ~URiveWidget() override;
    
protected:

    //~ BEGIN : UWidget Interface

#if WITH_EDITOR

    virtual const FText GetPaletteCategory() override;

#endif // WITH_EDITOR

    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

    virtual TSharedRef<SWidget> RebuildWidget() override;
    
    virtual void NativeConstruct() override;
    
    //~ END : UWidget Interface

    /**
     * Implementation(s)
     */

public:

    UFUNCTION(BlueprintCallable)
    void SetAudioEngine(URiveAudioEngine* InAudioEngine);

    UFUNCTION(BlueprintCallable)
    URiveArtboard* GetArtboard() const;
    
    /**
     * Attribute(s)
     */

    UPROPERTY(BlueprintAssignable, Category = Rive)
    FRiveReadyDelegate OnRiveReady;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive)
    FRiveDescriptor RiveDescriptor;
    
    UPROPERTY(BlueprintReadOnly, Transient, Category = Rive)
    TObjectPtr<URiveAudioEngine> RiveAudioEngine;
private:
    // Runtime objects
    UPROPERTY(Transient)
    TObjectPtr<URiveObject> RiveObject;
    
    void Setup();
    TSharedPtr<SRiveWidget> RiveWidget;
    FTimerHandle TimerHandle;

};
