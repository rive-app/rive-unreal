// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "UMG/RiveWidget.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveAudioEngine.h"
#include "Rive/RiveArtboard.h"
#include "TimerManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/PanelWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SRiveLeafWidget.h"

#define LOCTEXT_NAMESPACE "RiveWidget"

URiveWidget::URiveWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
  // SelfHitTestInvisible, the UUserWidget default, keeps this widget out of
  // Slate's hittest grid, and the grid is where navigation looks for the
  // next widget to focus. A pad could leave the artboard but never return.
  SetVisibilityInternal(ESlateVisibility::Visible);
}

URiveWidget::~URiveWidget()
{
  if (RiveWidget != nullptr)
  {
    RiveWidget->SetArtboard(nullptr);
  }

  RiveWidget.Reset();
}

#if WITH_EDITOR

const FText URiveWidget::GetPaletteCategory()
{
  return LOCTEXT("Rive", "RiveUI");
}

#endif // WITH_EDITOR

void URiveWidget::ReleaseSlateResources(bool bReleaseChildren)
{
  Super::ReleaseSlateResources(bReleaseChildren);

  if (RiveWidget != nullptr && bReleaseChildren)
  {
    RiveWidget->SetArtboard(nullptr);
  }

  RiveWidget.Reset();
}

TSharedRef<SWidget> URiveWidget::RebuildWidget()
{
  RiveWidget = SNew(SRiveLeafWidget).OwningWidget(this);

  // Without this the widget can never hold keyboard focus.
  SetIsFocusable(true);

  Setup();

  Initialize();

  return RiveWidget.ToSharedRef();
}

FReply URiveWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
  Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
  ERivePointerButton Button = ERivePointerButton::Primary;
  if (!RiveKeyToPointerButton(InMouseEvent.GetEffectingButton(), Button))
  {
    return FReply::Unhandled();
  }

  if (!IsValid(RiveArtboard))
  {
    return FReply::Unhandled();
  }

  // False either because nothing under the cursor wanted this button or
  // because the press missed. Both belong to the rest of the game, and
  // PointerDown settles them in one round trip.
  if (!RiveArtboard->PointerDown(
          InGeometry,
          RiveDescriptor,
          InMouseEvent,
          UWidgetLayoutLibrary::GetViewportScale(this),
          Button))
  {
    return FReply::Unhandled();
  }

  if (HeldButtonsTarget.Get() != RiveArtboard.Get())
  {
    HeldButtons = 0;
    HeldButtonsTarget = RiveArtboard.Get();
  }
  HeldButtons |= ButtonBit(Button);

  // Only on a press that hit something, so clicking through a transparent
  // artboard leaves focus where the player put it.
  return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}

FReply URiveWidget::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
  Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
  ERivePointerButton Button = ERivePointerButton::Primary;
  if (!RiveKeyToPointerButton(InMouseEvent.GetEffectingButton(), Button))
  {
    return FReply::Unhandled();
  }

  if (!IsValid(RiveArtboard))
  {
    return FReply::Unhandled();
  }

  // Primary is always delivered: a widget can appear under a cursor that is
  // already held down, and the artboard still wants that release.
  if (Button != ERivePointerButton::Primary &&
      ((HeldButtons & ButtonBit(Button)) == 0 ||
       HeldButtonsTarget.Get() != RiveArtboard.Get()))
  {
    return FReply::Unhandled();
  }
  HeldButtons &= static_cast<uint8>(~ButtonBit(Button));

  return RiveArtboard->PointerUp(
             InGeometry,
             RiveDescriptor,
             InMouseEvent,
             UWidgetLayoutLibrary::GetViewportScale(this),
             Button)
             ? FReply::Handled()
             : FReply::Unhandled();
}

FReply URiveWidget::NativeOnMouseMove(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
  Super::NativeOnMouseMove(InGeometry, InMouseEvent);

  if (IsValid(RiveArtboard))
  {
    RiveArtboard->PointerMove(
        InGeometry,
        RiveDescriptor,
        InMouseEvent,
        UWidgetLayoutLibrary::GetViewportScale(this));
  }

  return FReply::Handled();
}

void URiveWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
  Super::NativeOnMouseLeave(InMouseEvent);

  if (IsValid(RiveArtboard))
  {
    RiveArtboard->PointerExit(
        GetCachedGeometry(),
        RiveDescriptor,
        InMouseEvent,
        UWidgetLayoutLibrary::GetViewportScale(this));
  }
}

FReply URiveWidget::NativeOnTouchStarted(
    const FGeometry& InGeometry,
    const FPointerEvent& InGestureEvent)
{
  Super::NativeOnTouchStarted(InGeometry, InGestureEvent);

  if (IsValid(RiveArtboard))
  {
    RiveArtboard->PointerDown(
        GetCachedGeometry(),
        RiveDescriptor,
        InGestureEvent,
        UWidgetLayoutLibrary::GetViewportScale(this));
  }

  return FReply::Handled();
}

FReply URiveWidget::NativeOnTouchMoved(
    const FGeometry& InGeometry,
    const FPointerEvent& InGestureEvent)
{
  Super::NativeOnTouchMoved(InGeometry, InGestureEvent);

  if (IsValid(RiveArtboard))
  {
    RiveArtboard->PointerMove(
        GetCachedGeometry(),
        RiveDescriptor,
        InGestureEvent,
        UWidgetLayoutLibrary::GetViewportScale(this));
  }

  return FReply::Handled();
}

FReply URiveWidget::NativeOnTouchEnded(
    const FGeometry& InGeometry,
    const FPointerEvent& InGestureEvent)
{
  Super::NativeOnTouchEnded(InGeometry, InGestureEvent);

  if (IsValid(RiveArtboard))
  {
    RiveArtboard->PointerUp(
        GetCachedGeometry(),
        RiveDescriptor,
        InGestureEvent,
        UWidgetLayoutLibrary::GetViewportScale(this));
  }

  return FReply::Handled();
}

static bool IsNavigationKey(const FKeyEvent& InKeyEvent)
{
  return FSlateApplication::Get().GetNavigationDirectionFromKey(InKeyEvent) !=
         EUINavigation::Invalid;
}

static int32 GamepadDeviceId(const FInputEvent& InEvent)
{
  return InEvent.GetInputDeviceId().GetId();
}

bool URiveWidget::GamepadKey(const FKeyEvent& InKeyEvent, bool bPressed)
{
  if (!RiveArtboard->GamepadInput(
          InKeyEvent.GetKey(),
          bPressed ? 1.f : 0.f,
          GamepadDeviceId(InKeyEvent)))
  {
    return false;
  }

  // Left unhandled so Slate turns it into navigation, which moves focus
  // inside the artboard first (NativeOnNavigation). A listener on the same
  // button has already heard it through the gamepad event above.
  if (IsNavigationKey(InKeyEvent))
  {
    return false;
  }

  // The runtime doesn't say whether a listener took the press. With nothing
  // focused no listener can have, so B still backs out of the screen.
  return RiveArtboard->HasFocus();
}

FReply URiveWidget::NativeOnKeyDown(
    const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
  if (!IsValid(RiveArtboard))
  {
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
  }

  if (InKeyEvent.GetKey().IsGamepadKey())
  {
    return GamepadKey(InKeyEvent, true)
               ? FReply::Handled()
               : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
  }

  // Handled only when the runtime wanted it, so Escape still closes the
  // screen when nothing in the artboard is focused.
  if (RiveArtboard->KeyInput(InKeyEvent, true))
  {
    return FReply::Handled();
  }

  return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply URiveWidget::NativeOnKeyUp(
    const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
  if (!IsValid(RiveArtboard))
  {
    return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
  }

  if (InKeyEvent.GetKey().IsGamepadKey())
  {
    return GamepadKey(InKeyEvent, false)
               ? FReply::Handled()
               : Super::NativeOnKeyUp(InGeometry, InKeyEvent);
  }

  if (RiveArtboard->KeyInput(InKeyEvent, false))
  {
    return FReply::Handled();
  }

  return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FReply URiveWidget::NativeOnKeyChar(
    const FGeometry& InGeometry,
    const FCharacterEvent& InCharEvent)
{
  if (!IsValid(RiveArtboard))
  {
    return Super::NativeOnKeyChar(InGeometry, InCharEvent);
  }

  // Control characters are refused because several keys produce one as well
  // as a key event — backspace types \b — and inserting that alongside the
  // edit it already performed is what a text input reads as a stray glyph.
  const TCHAR Character = InCharEvent.GetCharacter();
  if (Character <= 0x1F || Character == 0x7F)
  {
    return Super::NativeOnKeyChar(InGeometry, InCharEvent);
  }

  // Its own OS message, so a printable key reaches both this and
  // NativeOnKeyDown.
  const FString Text = FString::Chr(Character);
  if (RiveArtboard->TextInput(Text))
  {
    return FReply::Handled();
  }

  return Super::NativeOnKeyChar(InGeometry, InCharEvent);
}

FReply URiveWidget::NativeOnAnalogValueChanged(
    const FGeometry& InGeometry,
    const FAnalogInputEvent& InAnalogEvent)
{
  if (IsValid(RiveArtboard))
  {
    RiveArtboard->GamepadInput(
        InAnalogEvent.GetKey(),
        InAnalogEvent.GetAnalogValue(),
        GamepadDeviceId(InAnalogEvent));
  }

  // Never handled: Slate's navigation config may turn a stick into
  // navigation (by default the left one), and the game still owns the
  // sticks unless its input mode says otherwise.
  return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

// Moves focus inside the artboard first and leaves only when it has nowhere
// further to go. The move is sent without waiting and the widget answers
// Stop; FinishNavigation replays the navigation through Slate if the
// artboard couldn't move. See "Navigation" in Plugins/Rive/docs/architecture.md
// for why, and for what the replay must guard against.
FNavigationReply URiveWidget::NativeOnNavigation(
    const FGeometry& InGeometry,
    const FNavigationEvent& InNavigationEvent,
    const FNavigationReply& InDefaultReply)
{
  // A replay from FinishNavigation: the artboard has already said it has
  // nowhere to go, so answer as a plain UMG widget would.
  if (bReplayingNavigation || !IsValid(RiveArtboard) ||
      !RiveArtboard->HasFocus())
  {
    return Super::NativeOnNavigation(
        InGeometry,
        InNavigationEvent,
        InDefaultReply);
  }

  const uint32 Sequence = ++NavigationSequence;
  const EUINavigation NavigationType = InNavigationEvent.GetNavigationType();
  const ENavigationGenesis Genesis = InNavigationEvent.GetNavigationGenesis();
  const uint32 UserIndex = InNavigationEvent.GetUserIndex();
  TWeakObjectPtr<URiveWidget> WeakThis(this);

  const bool bSent = RiveArtboard->MoveFocus(
      NavigationType,
      [WeakThis, Sequence, NavigationType, Genesis, UserIndex](bool bMoved) {
        if (URiveWidget* Widget = WeakThis.Get())
        {
          Widget->FinishNavigation(
              Sequence,
              NavigationType,
              Genesis,
              UserIndex,
              bMoved);
        }
      });
  if (!bSent)
  {
    // Not a move Rive makes: Slate's own rules, at once.
    return Super::NativeOnNavigation(
        InGeometry,
        InNavigationEvent,
        InDefaultReply);
  }

  // Hold focus here until the artboard answers.
  return FNavigationReply::Stop();
}

void URiveWidget::FinishNavigation(
    uint32 Sequence,
    EUINavigation NavigationType,
    ENavigationGenesis Genesis,
    uint32 UserIndex,
    bool bMoved)
{
  // Moved inside the artboard: the widget keeps focus, as Stop left it.
  if (bMoved)
  {
    return;
  }

  // A later navigation was sent after this one; its answer decides.
  if (Sequence != NavigationSequence)
  {
    return;
  }

  if (!FSlateApplication::IsInitialized())
  {
    return;
  }
  FSlateApplication& SlateApp = FSlateApplication::Get();

  // ProcessReply navigates from the user's current focus, so replay only
  // while that is still this widget.
  TSharedPtr<SWidget> SlateWidget = GetCachedWidget();
  if (!SlateWidget.IsValid() ||
      SlateApp.GetUserFocusedWidget(UserIndex) != SlateWidget)
  {
    return;
  }

  FWidgetPath PathToWidget;
  if (!SlateApp.FindPathToWidget(SlateWidget.ToSharedRef(), PathToWidget))
  {
    return;
  }

  TGuardValue<bool> Replaying(bReplayingNavigation, true);
  SlateApp.ProcessReply(
      PathToWidget,
      FReply::Handled().SetNavigation(NavigationType, Genesis),
      nullptr,
      nullptr,
      UserIndex);
}

FReply URiveWidget::NativeOnFocusReceived(
    const FGeometry& InGeometry,
    const FFocusEvent& InFocusEvent)
{
  FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);

  // Enter the artboard so its listeners have a focused node to hear events
  // through. FocusFirst leaves existing focus alone, and that check happens
  // on the server: the HasFocus cache could miss a ClearFocus queued this
  // frame. See "Slate focus and Rive focus" in
  // Plugins/Rive/docs/architecture.md.
  //
  // TODO: directional entry and restore (proposed/directional-focus-api).
  // See "Future work" in the same doc.
  if (IsValid(RiveArtboard))
  {
    RiveArtboard->FocusFirst();
  }

  return Reply;
}

void URiveWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
  Super::NativeOnFocusLost(InFocusEvent);

  if (IsValid(RiveArtboard))
  {
    // The releases of anything held now go to whatever took focus, often
    // the very press that moved it, so the artboard would keep it down.
    RiveArtboard->ReleaseGamepads();

    // Otherwise a text input keeps its caret blinking after the player has
    // clicked away.
    //
    // TODO: suspendFocus from the focus restore proposal would remember the
    // node instead of forgetting it. See "Future work" in the plugin docs.
    RiveArtboard->ClearFocus();
  }
}

void URiveWidget::SetAudioEngine(URiveAudioEngine* InRiveAudioEngine)
{
  RiveAudioEngine = InRiveAudioEngine;
  if (IsValid(RiveArtboard))
  {
    RiveArtboard->SetAudioEngine(InRiveAudioEngine);
  }
}

void URiveWidget::SetArtboard(URiveArtboard* InArtboard)
{
  RiveArtboard = InArtboard;
  if (IsValid(RiveArtboard))
  {
    RiveDescriptor.ArtboardName = RiveArtboard->GetArtboardName();
    if (RiveWidget.IsValid())
    {
      RiveWidget->SetArtboard(RiveArtboard);
    }
    if (IsValid(RiveAudioEngine))
    {
      RiveArtboard->SetAudioEngine(RiveAudioEngine);
    }
    EnterArtboardFocusIfFocused();
  }
}

URiveArtboard* URiveWidget::GetArtboard() const { return RiveArtboard; }

#if WITH_EDITOR
void URiveWidget::PostEditChangeChainProperty(
    FPropertyChangedChainEvent& PropertyChangedEvent)
{
  Super::PostEditChangeProperty(PropertyChangedEvent);

  const FName PropertyName = PropertyChangedEvent.GetPropertyName();
  const FName ActiveMemberNodeName = *PropertyChangedEvent.PropertyChain
                                          .GetActiveMemberNode()
                                          ->GetValue()
                                          ->GetName();

  if (PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, RiveFile) ||
      PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardName) ||
      PropertyName ==
          GET_MEMBER_NAME_CHECKED(FRiveDescriptor, StateMachineName) ||
      PropertyName ==
          GET_MEMBER_NAME_CHECKED(FRiveDescriptor, bAutoBindDefaultViewModel))
  {
    TArray<FString> ArtboardNames;
    if (RiveDescriptor.RiveFile)
    {
      for (const auto& Artboard : RiveDescriptor.RiveFile->ArtboardDefinitions)
      {
        ArtboardNames.Add(Artboard.Name);
      }
    }

    if (RiveDescriptor.ArtboardName.IsEmpty() ||
        (!ArtboardNames.Contains(RiveDescriptor.ArtboardName) &&
         !ArtboardNames.IsEmpty()))
    {
      RiveDescriptor.ArtboardName = ArtboardNames[0];
    }

    TArray<FString> StateMachineNames;
    if (RiveDescriptor.RiveFile)
    {
      for (const auto& Artboard : RiveDescriptor.RiveFile->ArtboardDefinitions)
      {
        if (Artboard.Name.Equals(RiveDescriptor.ArtboardName))
        {
          StateMachineNames = Artboard.StateMachineNames;
          break;
        }
      }
    }

    if (StateMachineNames.IsEmpty())
    {
      RiveDescriptor.StateMachineName = "None";
    }
    else if (
        RiveDescriptor.StateMachineName.IsEmpty() ||
        (RiveDescriptor.StateMachineName != TEXT("None") &&
         !StateMachineNames.Contains(RiveDescriptor.StateMachineName)))
    {
      RiveDescriptor.StateMachineName = StateMachineNames[0];
    }

    if (IsValid(RiveDescriptor.RiveFile))
    {
      if (RiveDescriptor.ArtboardName.IsEmpty())
      {
        UE_LOG(
            LogRive,
            Error,
            TEXT("URiveWidget::PostEditChangeChainProperty Selected "
                 "artboard is empty"));
        return;
      }

      RiveArtboard = URiveFile::MakeArtboardFromDescriptor(RiveDescriptor);

      if (IsValid(RiveAudioEngine))
      {
        RiveArtboard->SetAudioEngine(RiveAudioEngine);
      }

      RiveWidget->SetArtboard(RiveArtboard);
      RiveWidget->SetRiveDescriptor(RiveDescriptor);
    }
  }

  if (PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, Alignment) ||
      PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, FitType))
  {
    RiveWidget->SetRiveDescriptor(RiveDescriptor);
  }
}
#endif

void URiveWidget::Setup()
{
  // Don't load stuff for default object
  if (HasAnyFlags(RF_ClassDefaultObject))
  {
    return;
  }

  if (RiveArtboard == nullptr && RiveDescriptor.RiveFile != nullptr &&
      IsValid(RiveDescriptor.RiveFile))
  {
    if (RiveDescriptor.ArtboardName.IsEmpty())
    {
      UE_LOG(
          LogRive,
          Error,
          TEXT("URiveWidget::Setup Selected artboard is empty"));
      return;
    }

    RiveArtboard = URiveFile::MakeArtboardFromDescriptor(RiveDescriptor);
  }

  if (IsValid(RiveArtboard))
  {
    RiveDescriptor.ArtboardName = RiveArtboard->GetArtboardName();
    if (RiveWidget.IsValid())
    {
      RiveWidget->SetArtboard(RiveArtboard);
      RiveWidget->SetRiveDescriptor(RiveDescriptor);
    }
    if (IsValid(RiveAudioEngine))
    {
      RiveArtboard->SetAudioEngine(RiveAudioEngine);
    }
    EnterArtboardFocusIfFocused();
  }
}

void URiveWidget::EnterArtboardFocusIfFocused()
{
  // Swapping the artboard doesn't move Slate focus, so NativeOnFocusReceived
  // never runs for the new one.
  if (IsValid(RiveArtboard) && HasAnyUserFocus())
  {
    RiveArtboard->FocusFirst();
  }
}

void URiveWidget::SetRiveDescriptor(const FRiveDescriptor& newDescriptor)
{
  if (RiveDescriptor.FitType == ERiveFitType::Layout &&
      newDescriptor.FitType != ERiveFitType::Layout)
    IsChangingFromLayout = true;

  RiveDescriptor = newDescriptor;
  // reset artboard since we want to re create it.
  RiveArtboard = nullptr;

  Setup();
}
#undef LOCTEXT_NAMESPACE
