// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveCommandBuilder.h"
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

    UFUNCTION(BlueprintCallable, Category = "Rive|Testing")
    void DrawTestClear();

    UFUNCTION()
    TArray<FString> GetArtboardNamesForDropdown() const;

    UFUNCTION()
    TArray<FString> GetStateMachineNamesForDropdown() const;

    URiveArtboard* GetArtboard() const { return RiveArtboard; }

    URiveRenderTarget2D();

    void InitRiveRenderTarget2D();

    // Draws the artboard selected by RiveDescriptor
    UFUNCTION(BlueprintCallable, Category = "Rive | RenderTarget")
    void Draw();

    void Draw(DirectDrawCallback);

    virtual uint32 CalcTextureMemorySizeEnum(
        ETextureMipCount Enum) const override;
    virtual ETextureRenderTargetSampleCount GetSampleCount() const override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

private:
    void UpdateArtboardSize();
    // Draw the given Artboard using Descriptor for fit and alignment
    void Draw(URiveArtboard* InArtboard, FRiveDescriptor InDescriptor);

    UPROPERTY(Transient)
    TObjectPtr<URiveArtboard> RiveArtboard = nullptr;
    // Internal render target to rive
    TSharedPtr<FRiveRenderTarget> RenderTarget;
};
