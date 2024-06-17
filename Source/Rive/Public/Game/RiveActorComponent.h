// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"
#include "RiveTypes.h"
#include "Components/ActorComponent.h"
#include "Rive/RiveDescriptor.h"
#include "RiveActorComponent.generated.h"

class IRiveRenderer;
class URiveAudioEngine;
class URiveTexture;
class URiveArtboard;
class URiveFile;

UCLASS(ClassGroup = (Rive), Meta = (BlueprintSpawnableComponent), DisplayName=Rive)
class RIVE_API URiveActorComponent : public UActorComponent
{
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRiveReadyDelegate);
    
    GENERATED_BODY()

    /**
     * Structor(s)
     */

public:

    // Sets default values for this component's properties
    URiveActorComponent();

    //~ BEGIN : UActorComponent Interface

protected:

    // Called when the game starts
    virtual void BeginPlay() override;
    
public:

    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //~ END : UActorComponent Interface
    
    void Initialize();

    UPROPERTY(BlueprintAssignable, Category = Rive)
    FRiveReadyDelegate OnRiveReady;
    
    UFUNCTION(BlueprintCallable, Category = Rive)
    void ResizeRenderTarget(int32 InSizeX, int32 InSizeY);

    UFUNCTION(BlueprintCallable, Category = Rive)
    URiveArtboard* AddArtboard(URiveFile* InRiveFile, const FString& InArtboardName, const FString& InStateMachineName);

    UFUNCTION(BlueprintCallable, Category = Rive)
    void RemoveArtboard(URiveArtboard* InArtboard);
    
protected:
    void RiveReady(IRiveRenderer* InRiveRenderer);
    void OnResourceInitialized_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef& NewResource);
    
    /**
     * Attribute(s)
     */

public:
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Rive)
    FRiveDescriptor DefaultRiveDescriptor;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive, meta = (ClampMin = 1, UIMin = 1, ClampMax = 3840, UIMax = 3840))
    FIntPoint Size;
    
    UPROPERTY(BlueprintReadWrite, SkipSerialization, Transient, Category=Rive)
    TArray<URiveArtboard*> Artboards;

    UPROPERTY(BlueprintReadWrite, Transient, Category = Rive)
    TObjectPtr<URiveTexture> RiveTexture;

    UPROPERTY(BlueprintReadWrite, Transient, Category = Rive)
    TObjectPtr<URiveAudioEngine> RiveAudioEngine;
    
private:
    UFUNCTION()
    void OnDefaultArtboardTickRender(float DeltaTime, URiveArtboard* InArtboard);
    
    TSharedPtr<IRiveRenderTarget> RiveRenderTarget;
};
