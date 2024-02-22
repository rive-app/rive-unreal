// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"
#include "RiveTypes.h"
#include "Components/ActorComponent.h"
#include "RiveActorComponent.generated.h"

class URiveTexture;
class URiveArtboard;
class URiveFile;

USTRUCT(BlueprintType)
struct RIVE_API FRiveArtboardRenderData
{
    GENERATED_BODY()
public:

    FRiveArtboardRenderData(): FitType(ERiveFitType::Cover), Alignment(ERiveAlignment::Center), bIsReady(false)
    {
    }

    bool IsValid() const
    {
        return Artboard != nullptr;	
    }
	
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive)
    FString ArtboardName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive)
    FString StateMachineName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive)
    ERiveFitType FitType;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Rive)
    ERiveAlignment Alignment;

    UPROPERTY()
    URiveArtboard* Artboard;

    mutable bool bIsReady;
};

template <>
struct TStructOpsTypeTraits<FRiveArtboardRenderData> : public TStructOpsTypeTraitsBase2<FRiveArtboardRenderData>
{
    enum { WithCopy = false };
};

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
    URiveArtboard* InstantiateArtboard(URiveFile* InRiveFile, const FRiveArtboardRenderData& InArtboardRenderData);
    
protected:
    // void Tick_StateMachine(float DeltaTime, FRiveArtboardRenderData& InArtboardRenderData);
    // void Tick_Render(FRiveArtboardRenderData& InArtboardRenderData);
    
    /**
     * Attribute(s)
     */

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive, meta = (ClampMin = 1, UIMin = 1, ClampMax = 3840, UIMax = 3840))
    FIntPoint Size;
    
    UPROPERTY(BlueprintReadWrite, SkipSerialization, Transient, Category=Rive)
    TArray<FRiveArtboardRenderData> RenderObjects;

    UPROPERTY(BlueprintReadWrite, Transient)
    TObjectPtr<URiveTexture> RenderTarget;

private:
    UE::Rive::Renderer::IRiveRenderTargetPtr RiveRenderTarget;
};
