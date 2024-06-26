// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "rive/file.hpp"
#include "Rive/RiveDescriptor.h"
#include "Rive/RiveFile.h"
#include "RiveWidget.generated.h"

class URiveTextureObject;
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
    
    //~ END : UWidget Interface

    /**
     * Implementation(s)
     */

public:

    UFUNCTION(BlueprintCallable, Category = Rive)
    void SetAudioEngine(URiveAudioEngine* InAudioEngine);

    UFUNCTION(BlueprintCallable, Category = Rive)
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
    void OnRiveObjectReady();
    
    // Runtime objects
    UPROPERTY(Transient)
    TObjectPtr<URiveTextureObject> RiveObject;
    
    void Setup();
    TSharedPtr<SRiveWidget> RiveWidget;
    FTimerHandle TimerHandle;
    FDelegateHandle FrameHandle;

};
