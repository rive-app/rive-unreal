// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Rive/RiveDescriptor.h"
#include "Rive/RiveFile.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/file.hpp"
THIRD_PARTY_INCLUDES_END

#include "RiveWidget.generated.h"

struct FRiveStateMachine;
class URiveArtboard;
class SRiveLeafWidget;
class URiveAudioEngine;
class URiveFile;

/**
 *
 */
UCLASS()
class RIVE_API URiveWidget : public UUserWidget
{
  GENERATED_BODY()

public:
  URiveWidget(const FObjectInitializer& ObjectInitializer);

private:
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
  virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
  virtual FReply NativeOnTouchStarted(
      const FGeometry& InGeometry,
      const FPointerEvent& InGestureEvent) override;
  virtual FReply NativeOnTouchMoved(
      const FGeometry& InGeometry,
      const FPointerEvent& InGestureEvent) override;
  virtual FReply NativeOnTouchEnded(
      const FGeometry& InGeometry,
      const FPointerEvent& InGestureEvent) override;

  // Reached only while this widget holds focus, which it takes when a press
  // lands on something in the artboard (see NativeOnMouseButtonDown), or when
  // UMG navigation or SetFocus gives it focus.
  virtual FReply NativeOnKeyDown(
      const FGeometry& InGeometry,
      const FKeyEvent& InKeyEvent) override;
  virtual FReply NativeOnKeyUp(
      const FGeometry& InGeometry,
      const FKeyEvent& InKeyEvent) override;
  virtual FReply NativeOnKeyChar(
      const FGeometry& InGeometry,
      const FCharacterEvent& InCharEvent) override;
  virtual FReply NativeOnAnalogValueChanged(
      const FGeometry& InGeometry,
      const FAnalogInputEvent& InAnalogEvent) override;

  // Moves focus inside the artboard first, and leaves the widget only when
  // there is nothing further that way. See "Navigation" in the plugin docs
  // before changing anything here.
  virtual FNavigationReply NativeOnNavigation(
      const FGeometry& InGeometry,
      const FNavigationEvent& InNavigationEvent,
      const FNavigationReply& InDefaultReply) override;

  virtual FReply NativeOnFocusReceived(
      const FGeometry& InGeometry,
      const FFocusEvent& InFocusEvent) override;
  virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
  //~ END : UWidget Interface

  /**
   * Implementation(s)
   */

public:
  UFUNCTION(BlueprintCallable, Category = Rive)
  void SetAudioEngine(URiveAudioEngine* InRiveAudioEngine);

  UFUNCTION(BlueprintCallable, Category = Rive)
  void SetArtboard(URiveArtboard* InArtboard);

  UFUNCTION(BlueprintCallable, Category = Rive)
  URiveArtboard* GetArtboard() const;

  /**
   * Attribute(s)
   */

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

  // Gives a newly set artboard its entry focus when the widget already holds
  // Slate focus, as NativeOnFocusReceived would have.
  void EnterArtboardFocusIfFocused();

  // The game-thread half of a deferred navigation: replays it through
  // Slate when the artboard had nowhere to move. See NativeOnNavigation.
  void FinishNavigation(
      uint32 Sequence,
      EUINavigation NavigationType,
      ENavigationGenesis Genesis,
      uint32 UserIndex,
      bool bMoved);

  // Counts navigations sent to the artboard, so only the answer to the
  // latest one can make the widget leave.
  uint32 NavigationSequence = 0;

  // True while FinishNavigation replays a navigation through Slate, so
  // NativeOnNavigation answers with the widget's own UMG rules instead of
  // asking the artboard again.
  bool bReplayingNavigation = false;

  // Sends a gamepad button to the artboard and says whether the press was
  // the artboard's to keep. False for navigation directions, so they
  // can still become navigation.
  bool GamepadKey(const FKeyEvent& InKeyEvent, bool bPressed);

  UPROPERTY(Transient)
  TObjectPtr<URiveArtboard> RiveArtboard;

  UPROPERTY(Transient)
  TObjectPtr<URiveAudioEngine> RiveAudioEngine;

  TSharedPtr<SRiveLeafWidget> RiveWidget;
  FTimerHandle TimerHandle;

  FVector2f InitialArtboardSize;
  bool IsChangingFromLayout = false;

  // Non-primary buttons whose press reached the artboard, one bit each.
  // The release has to follow the press rather than ask again: by then the
  // cursor may have left the shape that took it, which is a normal gesture.
  uint8 HeldButtons = 0;
  TWeakObjectPtr<URiveArtboard> HeldButtonsTarget;

  static uint8 ButtonBit(ERivePointerButton Button)
  {
    return static_cast<uint8>(1u << static_cast<uint8>(Button));
  }
};
