// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "UObject/Object.h"
#include "RiveFile.generated.h"

class UTextureRenderTarget2D;
class URiveArtboard;

USTRUCT(Blueprintable)
struct FRiveStateMachineEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    FIntPoint MousePosition = FIntPoint(0, 0);
};

/**
 *
 */
UCLASS(Blueprintable)
class RIVE_API URiveFile : public UObject, public FTickableGameObject
{
    GENERATED_BODY()

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRiveStateMachineDelegate, FRiveStateMachineEvent, RiveStateMachineEvent);

    //~ BEGIN : FTickableGameObject Interface

public:

    virtual TStatId GetStatId() const override;

    virtual void Tick(float InDeltaSeconds) override;

    virtual bool IsTickable() const override;

    virtual bool IsTickableInEditor() const override
    {
        return true;
    }

    virtual ETickableTickType GetTickableTickType() const override
    {
        return ETickableTickType::Conditional;
    }

    //~ END : FTickableGameObject Interface

    //~ BEGIN : UObject Interface

public:

    virtual void PostLoad() override;

    //~ END : UObject Interface

    /**
     * Implementation(s)
     */

public:

    UFUNCTION(BlueprintPure, Category = Rive)
    UTextureRenderTarget2D* GetRenderTarget() const;

    UFUNCTION(BlueprintPure, Category = Rive)
    FLinearColor GetDebugColor() const;

    void Initialize();

    bool IsRendering() const;

    void RequestRendering();

    /**
     * Attribute(s)
     */

public:

    UPROPERTY(EditAnywhere, Category = Rive)
    TObjectPtr<UTextureRenderTarget2D> OverrideRenderTarget;

    UPROPERTY(EditAnywhere, Category = Rive)
    uint32 CountdownRenderingTicks = 5;

    // This Event is triggered any time new LiveLink data is available, including in the editor
    UPROPERTY(BlueprintAssignable, Category = "LiveLink")
    FRiveStateMachineDelegate OnRiveStateMachineDelegate;

    UPROPERTY()
    TArray<uint8> TempFileBuffer;

private:

    UPROPERTY(EditAnywhere, Category = Rive)
    FLinearColor DebugColor = FLinearColor::Black;

    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> RenderTarget;

    UPROPERTY()
    TObjectPtr<URiveArtboard> RiveArtboard;

    bool bIsInitialized = false;

    int32 CountdownRenderingTickCounter = 0;
};
