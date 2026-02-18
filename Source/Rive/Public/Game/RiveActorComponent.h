// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "RiveTypes.h"
#include "Components/ActorComponent.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveDescriptor.h"
#include "RiveActorComponent.generated.h"

class FRiveRenderer;
class URiveAudioEngine;
class URiveTexture;
class URiveArtboard;
class URiveFile;

UCLASS(ClassGroup = (Rive),
       Meta = (BlueprintSpawnableComponent),
       DisplayName = Rive)
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
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    //~ END : UActorComponent Interface

    void Initialize();

    UFUNCTION(BlueprintCallable, Category = Rive)
    void ResizeRenderTarget(int32 InSizeX, int32 InSizeY);

    UFUNCTION(BlueprintCallable, Category = Rive)
    void SetAudioEngine(URiveAudioEngine* InRiveAudioEngine);

#if WITH_EDITOR
    virtual void PostEditChangeChainProperty(
        FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Rive)
    FRiveDescriptor DefaultRiveDescriptor;

    UPROPERTY(BlueprintReadWrite,
              EditAnywhere,
              Category = Rive,
              meta = (ClampMin = 1, UIMin = 1, ClampMax = 3840, UIMax = 3840))
    FIntPoint Size;

    UPROPERTY(BlueprintReadWrite, Transient, Category = Rive)
    TObjectPtr<URiveAudioEngine> RiveAudioEngine;

    UFUNCTION(BlueprintPure, Category = Rive)
    UTexture* GetTexture() const;

private:
    UFUNCTION()
    TArray<FString> GetArtboardNamesForDropdown() const;

    UFUNCTION()
    TArray<FString> GetStateMachineNamesForDropdown() const;

    void InitializeAudioEngine();
    FDelegateHandle AudioEngineLambdaHandle;
    TObjectPtr<class URiveRenderTarget2D> RiveRenderTarget;
};
