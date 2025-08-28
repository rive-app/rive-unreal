// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Rive/RiveDescriptor.h"
#include "RiveRenderTarget2D.generated.h"

class URiveArtboard;
class FRiveRenderTarget;
/**
 *
 */
UCLASS()
class RIVE_API URiveRenderTarget2D : public UTextureRenderTarget2D
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,
              BlueprintReadOnly,
              Category = Rive,
              AssetRegistrySearchable)
    FRiveDescriptor RiveDescriptor;

    UPROPERTY(EditAnywhere,
              BlueprintReadOnly,
              Category = Rive,
              AssetRegistrySearchable)
    bool bShouldClear = true;

    virtual void PostLoad() override;

    UFUNCTION()
    TArray<FString> GetArtboardNamesForDropdown() const;

    UFUNCTION()
    TArray<FString> GetStateMachineNamesForDropdown() const;

    URiveRenderTarget2D();

    void InitRiveRenderTarget2D();

    // Draws the artboard selected by RiveDescriptor
    UFUNCTION(BlueprintCallable, Category = "Rive | RenderTarget")
    void Draw();

    virtual uint32 CalcTextureMemorySizeEnum(
        ETextureMipCount Enum) const override;
    virtual ETextureRenderTargetSampleCount GetSampleCount() const override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

private:
    // Draw the given Artboard using Descriptor for fit and alignment
    void Draw(URiveArtboard* InArtboard, FRiveDescriptor InDescriptor);

    UPROPERTY(Transient)
    TObjectPtr<URiveArtboard> RiveArtboard;
    // Internal render target to rive
    TSharedPtr<FRiveRenderTarget> RenderTarget;
};
