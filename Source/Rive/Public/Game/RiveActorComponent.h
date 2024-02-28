// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"
#include "RiveTypes.h"
#include "Components/ActorComponent.h"
#include "RiveActorComponent.generated.h"

class URiveTexture;
class URiveArtboard;
class URiveFile;

UCLASS(ClassGroup = (Custom), Meta = (BlueprintSpawnableComponent))
class RIVE_API URiveActorComponent : public UActorComponent
{
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
    
    void InitializeRenderTarget(int32 SizeX, int32 SizeY);

    UFUNCTION(BlueprintCallable)
    void ResizeRenderTarget(int32 InSizeX, int32 InSizeY);

    UFUNCTION(BlueprintCallable)
    URiveArtboard* InstantiateArtboard(URiveFile* InRiveFile, const FString& InArtboardName, const FString& InStateMachineName);
    
protected:
    void OnResourceInitialized_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef& NewResource) const;
    /**
     * Attribute(s)
     */

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive, meta = (ClampMin = 1, UIMin = 1, ClampMax = 3840, UIMax = 3840))
    FIntPoint Size;
    
    UPROPERTY(BlueprintReadWrite, SkipSerialization, Transient, Category=Rive)
    TArray<URiveArtboard*> RenderObjects;

    UPROPERTY(BlueprintReadWrite, Transient)
    TObjectPtr<URiveTexture> RenderTarget;

private:
    UE::Rive::Renderer::IRiveRenderTargetPtr RiveRenderTarget;
};
