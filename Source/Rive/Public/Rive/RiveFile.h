// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"
#include "Assets/URFile.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RiveFile.generated.h"

USTRUCT(Blueprintable)
struct RIVE_API FRiveStateMachineEvent
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
struct RIVE_API FRiveAlignment
{
    GENERATED_BODY()

public:

    inline static FVector2f TopLeft = FVector2f(-1.f, -1.f);
    
    inline static FVector2f TopCenter = FVector2f(0.f, -1.f);
    
    inline static FVector2f TopRight = FVector2f(1.f, -1.f);
    
    inline static FVector2f CenterLeft = FVector2f(-1.f, 0.f);
    
    inline static FVector2f Center = FVector2f(0.f, 0.f);
    
    inline static FVector2f CenterRight = FVector2f(1.f, 0.f);
    
    inline static FVector2f BottomLeft = FVector2f(-1.f, 1.f);
    
    inline static FVector2f BottomCenter = FVector2f(0.f, 1.f);
    
    inline static FVector2f BottomRight = FVector2f(1.f, 1.f);
};

UENUM(BlueprintType)
enum class ERiveAlignment : uint8
{
    TopLeft = 0,
    TopCenter = 1,
    TopRight = 2,
    CenterLeft = 3,
    Center = 4,
    CenterRight = 5,
    BottomLeft = 6,
    BottomCenter = 7,
    BottomRight = 8,
};

UENUM(BlueprintType)
enum class ERiveBlendMode : uint8
{
    SE_BLEND_Opaque = 0,
    SE_BLEND_Masked,
    SE_BLEND_Translucent,
    SE_BLEND_Additive,
    SE_BLEND_Modulate,
    SE_BLEND_MaskedDistanceField,
    SE_BLEND_MaskedDistanceFieldShadowed,
    SE_BLEND_TranslucentDistanceField,
    SE_BLEND_TranslucentDistanceFieldShadowed,
    SE_BLEND_AlphaComposite,
    SE_BLEND_AlphaHoldout,
};

/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class RIVE_API URiveFile : public UTextureRenderTarget2D, public FTickableGameObject
{
    GENERATED_BODY()

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRiveStateMachineDelegate, FRiveStateMachineEvent, RiveStateMachineEvent);

    //~ BEGIN : FTickableGameObject Interface

public:

    URiveFile();

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

    //~ BEGIN : UTexture Interface

    virtual uint32 CalcTextureMemorySizeEnum(ETextureMipCount Enum) const override;

    //~ END : UObject UTexture

    //~ BEGIN : UObject Interface

public:

    virtual void PostLoad() override;

#if WITH_EDITOR

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif // WITH_EDITOR

    //~ END : UObject Interface

    /**
     * Implementation(s)
     */

public:

    UFUNCTION(BlueprintPure, Category = Rive)
    FLinearColor GetDebugColor() const;

    UFUNCTION(BlueprintPure, Category = Rive)
    FVector2f GetLocalCoordinates(const FVector2f& InScreenPosition, const FBox2f& InScreenRect) const;

    // TODO. We need function in URiveFile to calculate it , based on RiveFitType
    FIntPoint CalculateRenderTextureSize(const FIntPoint& InViewportSize) const;

    // TODO. We need function in URiveFile to calculate it , based on RiveAlignment
    FIntPoint CalculateRenderTexturePosition(const FIntPoint& InViewportSize) const;

    FVector2f GetRiveAlignment() const;

    ESimpleElementBlendMode GetSimpleElementBlendMode() const;

    void BeginInput()
    {
        bIsReceivingInput = true;
    }

    void EndInput()
    {
        bIsReceivingInput = false;
    }

    void Initialize();

    void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

    TSubclassOf<UUserWidget> GetWidgetClass() const { return WidgetClass; }

    UE::Rive::Core::FURArtboard* GetArtboard() const;

private:

    bool LoadNativeFile();

    /**
     * Attribute(s)
     */

public:

    UPROPERTY(EditAnywhere, Category = Rive)
    TObjectPtr<UTextureRenderTarget2D> CopyRenderTarget;

    // This Event is triggered any time new LiveLink data is available, including in the editor
    UPROPERTY(BlueprintAssignable, Category = "LiveLink")
    FRiveStateMachineDelegate OnRiveStateMachineDelegate;

    UPROPERTY()
    TArray<uint8> TempFileBuffer;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> RenderTarget;

    // TODO. REMOVE IT!!, just for testing
    UPROPERTY(EditAnywhere, Category = Rive)
    bool bUseViewportClientTestProperty = true;
    
private:

    UPROPERTY(EditAnywhere, Category = Rive)
    FLinearColor DebugColor = FLinearColor::Transparent;

    UPROPERTY(EditAnywhere, Category = Rive)
    ERiveFitType RiveFitType = ERiveFitType::Fill;

    UPROPERTY(EditAnywhere, Category = Rive)
    ERiveAlignment RiveAlignment = ERiveAlignment::Center;

    UPROPERTY(EditAnywhere, Category = Rive)
    ERiveBlendMode RiveBlendMode = ERiveBlendMode::SE_BLEND_Opaque;

    UPROPERTY(EditAnywhere, Category = Rive)
    bool bIsRendering = true;

    UPROPERTY(EditAnywhere, Category=Rive)
    TSubclassOf<UUserWidget> WidgetClass;

    bool bIsInitialized = false;

    bool bIsReceivingInput = false;

    UE::Rive::Renderer::IRiveRenderTargetPtr RiveRenderTarget;

    bool bDrawOnceTest = false;

    UE::Rive::Assets::FURFilePtr UnrealRiveFile;
};
