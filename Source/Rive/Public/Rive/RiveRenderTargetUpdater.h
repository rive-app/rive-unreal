// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RiveRenderTargetUpdater.generated.h"

class URiveRenderTarget2D;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class URiveRenderTargetUpdater : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    URiveRenderTargetUpdater();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rive")
    bool bAutoDrawRenderTarget = false;

    // Requires 'Support UV From Hit Results' to be enabled in project settings
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Rive")
    bool bEnableMouseEvents;

    // If set, Invert the X hit coordinate.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Rive")
    bool bInvertUVX = false;

    // If set, Invert the Y hit coordinate.
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Rive")
    bool bInvertUVY = false;

    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Rive")
    double LineTraceDistance = 1000000.0;
    // Index of the player controller to use for mouse events
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rive")
    int32 PlayerIndex = 0;

    // UV Channel to use with "UGameplayStatics::FindCollisionUV"
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rive")
    int32 UVChannel = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rive")
    TObjectPtr<URiveRenderTarget2D> RenderTargetToUpdate;

protected:
    UFUNCTION()
    void OnMouseEnter(AActor* TouchedActor);
    UFUNCTION()
    void OnMouseLeave(AActor* TouchedActor);
    UFUNCTION()
    void OnMouseUp(AActor* TouchedActor, FKey ButtonPressed);
    UFUNCTION()
    void OnMouseDown(AActor* TouchedActor, FKey ButtonPressed);

    bool bIsMouseWithinView = false;

private:
    bool LineTraceWithUVResult(FVector2D& OutUVHit);
};
