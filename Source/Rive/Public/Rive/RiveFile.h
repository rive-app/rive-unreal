// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "UObject/Object.h"
#include "RiveFile.generated.h"

namespace UE::Rive::Renderer
{
    class IRiveRenderTarget;
}

class UTextureRenderTarget2D;
class URiveArtboard;

USTRUCT(Blueprintable)
struct FRiveStateMachineEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    FIntPoint MousePosition = FIntPoint(0, 0);
};

UENUM(BlueprintType)
enum class ERiveFitType : uint8
{
    Fill = 0,
    Contain = 1,
    Cover = 2,
    FitWidth = 3,
    FitHeight = 4,
    None = 5,
    ScaleDown = 6
};

USTRUCT()
struct FRiveAlignment
{
    GENERATED_BODY()

public:
    inline static FVector2f TopLeft = FVector2f(-1.0f, -1.0f);
    inline static FVector2f TopCenter = FVector2f(0.0f, -1.0f);
    inline static FVector2f TopRight = FVector2f(1.0f, -1.0f);
    inline static FVector2f CenterLeft = FVector2f(-1.0f, 0.0f);
    inline static FVector2f Center = FVector2f(0.0f, 0.0f);
    inline static FVector2f CenterRight = FVector2f(1.0f, 0.0f);
    inline static FVector2f BottomLeft = FVector2f(-1.0f, 1.0f);
    inline static FVector2f BottomCenter = FVector2f(0.0f, 1.0f);
    inline static FVector2f BottomRight = FVector2f(1.0f, 1.0f);
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

    void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

    TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }

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
    FLinearColor DebugColor = FLinearColor::Transparent;

    UPROPERTY(EditAnywhere, Category = Rive)
    ERiveFitType RiveFitType = ERiveFitType::Contain;

    UPROPERTY(EditAnywhere, Category = Rive)
    FVector2f RiveAlignment = FRiveAlignment::Center;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> RenderTarget;

    UPROPERTY()
    TObjectPtr<URiveArtboard> RiveArtboard;

    UPROPERTY(EditAnywhere, Category=Rive)
    TSubclassOf<UUserWidget> WidgetClass;

    bool bIsInitialized = false;

    int32 CountdownRenderingTickCounter = 0;

    TSharedPtr<UE::Rive::Renderer::IRiveRenderTarget> RiveRenderTarget;

    bool bDrawOnceTest = false;
};
