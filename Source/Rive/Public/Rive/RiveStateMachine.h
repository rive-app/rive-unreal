// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "RiveCommandBuilder.h"

#include <atomic>

#if WITH_RIVE

struct FRiveDescriptor;
struct FRiveCommandBuilder;
#include "Input/Events.h"
#include "Layout/Geometry.h"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/command_queue.hpp"
#include "rive/event_report.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

class URiveViewModel;

// Maps a Slate mouse key onto the button Rive listens for. False for keys the
// runtime has no notion of, such as the thumb buttons, so a host can leave
// those to the rest of the game.
FORCEINLINE bool RiveKeyToPointerButton(
    const FKey& InKey,
    ERivePointerButton& OutButton)
{
  if (InKey == EKeys::LeftMouseButton)
  {
    OutButton = ERivePointerButton::Primary;
    return true;
  }
  if (InKey == EKeys::RightMouseButton)
  {
    OutButton = ERivePointerButton::Secondary;
    return true;
  }
  if (InKey == EKeys::MiddleMouseButton)
  {
    OutButton = ERivePointerButton::Middle;
    return true;
  }
  return false;
}

/**
 * Represents a Rive State Machine from an Artboard. A State Machine contains
 * Inputs.
 */
struct RIVE_API FRiveStateMachine : public TSharedFromThis<FRiveStateMachine>
{
  bool IsValid() const
  {
    return NativeStateMachineHandle != RIVE_NULL_HANDLE && bIsValid;
  }

  void Destroy(FRiveCommandBuilder& CommandBuilder);
  uint64_t Initialize(
      FRiveCommandBuilder& CommandBuilder,
      rive::ArtboardHandle InOwningArtboardHandle,
      const FString& StateMachineName);

  void Advance(FRiveCommandBuilder&, float InSeconds);

  uint32 GetInputCount() const;

  bool PointerDown(
      const FRiveDescriptor& InDescriptor,
      const FVector2D& NormalLocationOnSurface,
      ERivePointerButton Button = ERivePointerButton::Primary);

  bool PointerMove(
      const FRiveDescriptor& InDescriptor,
      const FVector2D& NormalLocationOnSurface);

  bool PointerUp(
      const FRiveDescriptor& InDescriptor,
      const FVector2D& NormalLocationOnSurface,
      ERivePointerButton Button = ERivePointerButton::Primary);

  bool PointerExit(
      const FRiveDescriptor& InDescriptor,
      const FVector2D& NormalLocationOnSurface);

  // A press of a non-primary button that nothing under the cursor listens
  // for never reaches the state machine and reports no hit, so a host can
  // build an FReply from the return value alone. That question and the press
  // itself are answered in one round trip.
  bool PointerDown(
      const FGeometry& MyGeometry,
      const FRiveDescriptor& InDescriptor,
      const FPointerEvent& MouseEvent,
      float DPI,
      ERivePointerButton Button = ERivePointerButton::Primary);

  bool PointerMove(
      const FGeometry& MyGeometry,
      const FRiveDescriptor& InDescriptor,
      const FPointerEvent& MouseEvent,
      float DPI);

  bool PointerUp(
      const FGeometry& MyGeometry,
      const FRiveDescriptor& InDescriptor,
      const FPointerEvent& MouseEvent,
      float DPI,
      ERivePointerButton Button = ERivePointerButton::Primary);

  bool PointerExit(
      const FGeometry& InGeometry,
      const FRiveDescriptor& InDescriptor,
      const FPointerEvent& MouseEvent,
      float DPI);

  // CommandQueue has no key or text command, so these reach the state machine
  // instance through a server-side callback. Both return whether the runtime
  // handled the event, which is what lets a host build an honest FReply.
  bool KeyInput(const FKeyEvent& InKeyEvent, bool bPressed);

  bool KeyInput(
      FKey InKey,
      FModifierKeysState InModifiers,
      bool bPressed,
      bool bRepeat);

  // Committed text — a typed character, an IME commit, a paste. Separate from
  // KeyInput because the OS has already resolved shift, layout and dead keys.
  bool TextInput(const FString& InText);

  // Fire and forget: nothing is decided on the strength of it.
  void ClearFocus();

  // Whether anything in the artboard holds focus. Read from a cache so it
  // never waits on the render thread; it can be a frame behind. See "Focus
  // state cache" in the plugin docs.
  bool HasFocus() const { return CachedHasFocus->load(); }

  // Focuses the first stop in tab order, only when nothing is focused, so it
  // never moves focus the player already has. Fire and forget.
  void FocusFirst();

  // Moves focus one step inside the artboard. Doesn't wait: OnResult runs on
  // the game thread with whether focus moved. Returns false, without calling
  // OnResult, for a navigation type Rive has no move for.
  bool MoveFocus(
      EUINavigation Navigation,
      TFunction<void(bool bMoved)> OnResult);

  // A gamepad button or axis as Unreal names it, sent as a W3C standard
  // gamepad change. False for keys with no standard slot, before anything is
  // sent. Does not wait: analog input arrives every frame a stick is held.
  bool GamepadInput(const FKey& InKey, float Value, int32 DeviceId);

  // Returns every pad to rest. For a host that stops hearing the pad while
  // it is held: once focus moves away the release goes to another widget,
  // and the artboard would keep the button down.
  void ReleaseGamepads();

  void BindViewModel(TObjectPtr<URiveViewModel> ViewModel);

  void SetStateMachineSettled(bool inStateMachineSettled);
  void OnStateMachineError(uint64_t requestId, std::string error);

  bool IsStateMachineSettled() const { return bStateMachineSettled; }

  const FString& GetStateMachineName() const { return StateMachineName; }

  rive::StateMachineHandle GetNativeStateMachineHandle() const
  {
    return NativeStateMachineHandle;
  }

  TArray<FString> BoolInputNames;
  TArray<FString> NumberInputNames;
  TArray<FString> TriggerInputNames;

  void SetValid(bool InValid) { bIsValid = InValid; }

private:
  FString StateMachineName;
  rive::StateMachineHandle NativeStateMachineHandle = RIVE_NULL_HANDLE;
  static rive::EventReport NullEvent;
  bool bStateMachineSettled = false;
  bool bIsValid = true;

  // Written on the server, read by HasFocus. Shared so a callback still in
  // flight when this state machine goes away writes somewhere valid.
  TSharedRef<std::atomic<bool>> CachedHasFocus = MakeShared<std::atomic<bool>>(
      false);

  // What each pad has last been told, so the first event from a pad can
  // announce it and an unchanged value is not sent again.
  struct FGamepadState
  {
    TArray<float> Buttons;
    TArray<float> Axes;
  };
  TMap<int32, FGamepadState> Gamepads;
};
