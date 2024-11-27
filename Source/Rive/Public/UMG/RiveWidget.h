// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "rive/file.hpp"
#include "Rive/RiveDescriptor.h"
#include "Rive/RiveFile.h"
#include "RiveWidget.generated.h"

class FRiveStateMachine;
class URiveTextureObject;
class URiveArtboard;
class URiveTexture;
class SRiveWidget;
class URiveAudioEngine;
class URiveFile;

/**
 *
 */
UCLASS()
class RIVE_API URiveWidget : public UUserWidget
{
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRiveReadyDelegate);

    GENERATED_BODY()

    virtual ~URiveWidget() override;

protected:
    //~ BEGIN : UWidget Interface

#if WITH_EDITOR
    virtual const FText GetPaletteCategory() override;
#endif // WITH_EDITOR

    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;
    //~ END : UWidget Interface

    /**
     * Implementation(s)
     */

public:
    UFUNCTION(BlueprintCallable, Category = Rive)
    void SetAudioEngine(URiveAudioEngine* InRiveAudioEngine);

    UFUNCTION(BlueprintCallable, Category = Rive)
    URiveArtboard* GetArtboard() const;

    UFUNCTION()
    void OnSWidgetSizeChanged(const FVector2D& NewSize);

    UFUNCTION()
    void CheckArtboardSize();

    /**
     * Attribute(s)
     */

    UPROPERTY(BlueprintAssignable, Category = Rive)
    FRiveReadyDelegate OnRiveReady;

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Rive)
    FRiveDescriptor RiveDescriptor;

    UFUNCTION(BlueprintCallable, Category = Rive)
    void SetRiveDescriptor(const FRiveDescriptor& newDescriptor);

#if WITH_EDITOR
    virtual void PostEditChangeChainProperty(
        FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
private:
    void Setup();

    UFUNCTION()
    void OnRiveObjectReady();

    UFUNCTION()
    TArray<FString> GetArtboardNamesForDropdown() const;

    UFUNCTION()
    TArray<FString> GetStateMachineNamesForDropdown() const;
    FReply OnInput(const FGeometry& MyGeometry,
                   const FPointerEvent& MouseEvent,
                   const TFunction<bool(const FVector2f&, FRiveStateMachine*)>&
                       InStateMachineInputCallback);

    UPROPERTY(Transient)
    TObjectPtr<URiveTextureObject> RiveTextureObject;

    TSharedPtr<SRiveWidget> RiveWidget;
    FTimerHandle TimerHandle;

    FVector2f InitialArtboardSize;
    bool IsChangingFromLayout = false;
};
