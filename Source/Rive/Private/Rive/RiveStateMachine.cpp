// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "Rive/RiveStateMachine.h"
#include "Rive/RiveViewModel.h"

#include "IRiveRendererModule.h"
#include "RiveCommandBuilder.h"
#include "RiveRenderer.h"
#include "RiveTypeConversions.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Logs/RiveLog.h"
#include "InputCoreTypes.h"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/command_queue.hpp"
#include "rive/command_server.hpp"
#include "rive/input/focus_manager.hpp"
#include "rive/input/focusable.hpp"
#include "rive/input/gamepad_batch.hpp"
#include "rive/input/standard_gamepad.hpp"
#include "Rive/RiveDescriptor.h"
#include "Stats/RiveStats.h"
#include "Rive/RiveUtils.h"
#include "RenderingThread.h"
#include "Async/Async.h"

#include <atomic>

#if WITH_RIVE

class FRiveStateMachineListener final
    : public rive::CommandQueue::StateMachineListener
{
public:
  FRiveStateMachineListener(TWeakPtr<FRiveStateMachine> ListeningStateMachine)
      : ListeningStateMachine(MoveTemp(ListeningStateMachine))
  {}

  virtual void onStateMachineError(
      const rive::StateMachineHandle,
      uint64_t requestId,
      std::string error) override
  {
    check(IsInGameThread());
    if (auto StrongStateMachine = ListeningStateMachine.Pin();
        StrongStateMachine.IsValid())
    {
      StrongStateMachine->OnStateMachineError(requestId, std::move(error));
    }
  }

  virtual void onStateMachineDeleted(
      const rive::StateMachineHandle Handle,
      uint64_t requestId) override
  {
    check(IsInGameThread());
    if (auto StrongStateMachine = ListeningStateMachine.Pin();
        StrongStateMachine.IsValid())
    {
      UE_LOG(
          LogRive,
          Display,
          TEXT("Rive StateMachine Named %s deleted"),
          *StrongStateMachine->GetStateMachineName());
    }
    else
    {
      UE_LOG(
          LogRive,
          VeryVerbose,
          TEXT("Rive StateMachine Handle %p deleted"),
          Handle);
    }

    delete this;
  }
  // requestId in this case is the specific request that caused the
  // statemachine to settle
  virtual void onStateMachineSettled(
      const rive::StateMachineHandle,
      uint64_t requestId) override
  {
    check(IsInGameThread());
    if (auto StrongStateMachine = ListeningStateMachine.Pin();
        StrongStateMachine.IsValid())
    {
      StrongStateMachine->SetStateMachineSettled(true);
    }
  }

private:
  TWeakPtr<FRiveStateMachine> ListeningStateMachine;
};

rive::EventReport FRiveStateMachine::NullEvent = rive::EventReport(
    nullptr,
    0.f);

void FRiveStateMachine::Destroy(FRiveCommandBuilder& CommandBuilder)
{
  CommandBuilder.DestroyStateMachine(NativeStateMachineHandle);
}

uint64_t FRiveStateMachine::Initialize(
    FRiveCommandBuilder& CommandBuilder,
    rive::ArtboardHandle InOwningArtboardHandle,
    const FString& InStateMachineName)
{
  uint64_t CreateRequestId = 0;
  bStateMachineSettled = false;
  if (InStateMachineName.IsEmpty())
  {
    NativeStateMachineHandle = CommandBuilder.CreateDefaultStateMachine(
        InOwningArtboardHandle,
        new FRiveStateMachineListener(AsWeak()),
        &CreateRequestId);
  }
  else
  {
    NativeStateMachineHandle = CommandBuilder.CreateStateMachine(
        InOwningArtboardHandle,
        InStateMachineName,
        new FRiveStateMachineListener(AsWeak()),
        &CreateRequestId);
    StateMachineName = InStateMachineName;
  }

  return CreateRequestId;
}

void FRiveStateMachine::Advance(
    FRiveCommandBuilder& CommandBuilder,
    float InSeconds)
{
  SCOPED_NAMED_EVENT_TEXT(TEXT("FRiveStateMachine::Advance"), FColor::White);
  DECLARE_SCOPE_CYCLE_COUNTER(
      TEXT("FRiveStateMachine::Advance"),
      STAT_STATEMACHINE_ADVANCE,
      STATGROUP_Rive);
  CommandBuilder.AdvanceStateMachine(NativeStateMachineHandle, InSeconds);

  // Listeners apply focus changes during the advance, so refresh the cache
  // after it.
  const rive::StateMachineHandle Handle = NativeStateMachineHandle;
  CommandBuilder.RunOnce([Handle,
                          Cache = CachedHasFocus](rive::CommandServer* Server) {
    if (rive::StateMachineInstance* Instance = Server->getStateMachineInstance(
            Handle))
    {
      Cache->store(Instance->focusState().hasFocus);
    }
  });
}

uint32 FRiveStateMachine::GetInputCount() const { return 0; }

bool FRiveStateMachine::PointerDown(
    const FRiveDescriptor& InDescriptor,
    const FVector2D& NormalLocationOnSurface,
    ERivePointerButton Button)
{
  bStateMachineSettled = false;
  auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
  CommandBuilder.StateMachineMouseDown(
      NativeStateMachineHandle,
      {.fit = RiveFitTypeToFit(InDescriptor.FitType),
       .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
       // Normalized Bounds 0-1
       .screenBounds = {1, 1},
       .position =
           {static_cast<float>(NormalLocationOnSurface.X),
            static_cast<float>(NormalLocationOnSurface.Y)},
       .scaleFactor = InDescriptor.ScaleFactor,
       .button = RivePointerButtonToPointerButton(Button)});
  return false;
}

bool FRiveStateMachine::PointerMove(
    const FRiveDescriptor& InDescriptor,
    const FVector2D& NormalLocationOnSurface)
{
  bStateMachineSettled = false;
  auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
  CommandBuilder.StateMachineMouseMove(
      NativeStateMachineHandle,
      {.fit = RiveFitTypeToFit(InDescriptor.FitType),
       .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
       // Normalized Bounds 0-1
       .screenBounds = {1, 1},
       .position =
           {static_cast<float>(NormalLocationOnSurface.X),
            static_cast<float>(NormalLocationOnSurface.Y)},
       .scaleFactor = InDescriptor.ScaleFactor});
  return false;
}

bool FRiveStateMachine::PointerUp(
    const FRiveDescriptor& InDescriptor,
    const FVector2D& NormalLocationOnSurface,
    ERivePointerButton Button)
{
  bStateMachineSettled = false;
  auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
  CommandBuilder.StateMachineMouseUp(
      NativeStateMachineHandle,
      {.fit = RiveFitTypeToFit(InDescriptor.FitType),
       .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
       // Normalized Bounds 0-1
       .screenBounds = {1, 1},
       .position =
           {static_cast<float>(NormalLocationOnSurface.X),
            static_cast<float>(NormalLocationOnSurface.Y)},
       .scaleFactor = InDescriptor.ScaleFactor,
       .button = RivePointerButtonToPointerButton(Button)});
  return false;
}

bool FRiveStateMachine::PointerExit(
    const FRiveDescriptor& InDescriptor,
    const FVector2D& NormalLocationOnSurface)
{
  bStateMachineSettled = false;
  auto& CommandBuilder = IRiveRendererModule::GetCommandBuilder();
  CommandBuilder.StateMachineMouseOut(
      NativeStateMachineHandle,
      {.fit = RiveFitTypeToFit(InDescriptor.FitType),
       .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
       // Normalized Bounds 0-1
       .screenBounds = {1, 1},
       .position =
           {static_cast<float>(NormalLocationOnSurface.X),
            static_cast<float>(NormalLocationOnSurface.Y)},
       .scaleFactor = InDescriptor.ScaleFactor});
  return false;
}

// Blocks for the server's answer. Queued via runOnce to stay FIFO with pending
// commands; the render command forces an early processCommands(), since the
// per-frame drain can't run while the game thread blocks here.
template <typename ResultType>
static ResultType RunOnServerAndWait(
    TFunction<ResultType(rive::CommandServer*)> Work,
    ResultType DefaultResult,
    const TCHAR* TimeoutContext)
{
  FRiveRenderer* Renderer = IRiveRendererModule::Get().GetRenderer();
  check(Renderer);

  if (IsInRenderingThread())
  {
    return Work(Renderer->GetCommandServer());
  }

  struct FPending
  {
    FEventRef Done;
    std::atomic<ResultType> Result;
  };
  // Shared so a timed-out wait can return while the callback still runs.
  // Seeded here because a local class cannot use an enclosing function's
  // parameter as a member initialiser.
  auto Pending = MakeShared<FPending>();
  Pending->Result = DefaultResult;

  Renderer->GetCommandBuilder().RunOnceImmediate(
      [Pending, Work](rive::CommandServer* Server) {
        Pending->Result = Work(Server);
        Pending->Done->Trigger();
      });

  ENQUEUE_RENDER_COMMAND(RiveServerWaitFlush)
  ([Renderer](FRHICommandListImmediate&) {
    if (auto* Server = Renderer->GetCommandServer())
    {
      Server->processCommands();
    }
  });

  constexpr uint32 TimeoutMs = 100;
  if (!Pending->Done->Wait(TimeoutMs))
  {
    UE_LOG(
        LogRive,
        Warning,
        TEXT("%s timed out waiting on command server; treating as "
             "unhandled"),
        TimeoutContext);
    return DefaultResult;
  }
  return Pending->Result;
}

static rive::HitResult SendPointerEventAndWait(
    rive::StateMachineHandle Handle,
    const rive::CommandQueue::PointerEvent& Event,
    rive::HitResult (rive::CommandServer::*Send)(
        rive::StateMachineHandle,
        const rive::CommandQueue::PointerEvent&))
{
  return RunOnServerAndWait<rive::HitResult>(
      [Handle, Event, Send](rive::CommandServer* Server) {
        return (Server->*Send)(Handle, Event);
      },
      rive::HitResult::none,
      TEXT("Pointer event"));
}

bool FRiveStateMachine::PointerDown(
    const FGeometry& InGeometry,
    const FRiveDescriptor& InDescriptor,
    const FPointerEvent& InMouseEvent,
    float DPI,
    ERivePointerButton Button)
{
  bStateMachineSettled = false;
  float ScaleFactor = 1.0f;

  FVector2D Position = USlateBlueprintLibrary::AbsoluteToLocal(
      InGeometry,
      InMouseEvent.GetScreenSpacePosition());
  FVector2D ScreenBounds = InGeometry.GetLocalSize();

  if (InDescriptor.FitType == ERiveFitType::Layout)
  {
    ScaleFactor = InDescriptor.ScaleFactor;
    if (InDescriptor.bScaleLayoutByDPI)
      ScaleFactor *= DPI;
    Position *= DPI;
    ScreenBounds *= DPI;
  }

  const rive::CommandQueue::PointerEvent Event{
      .fit = RiveFitTypeToFit(InDescriptor.FitType),
      .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
      .screenBounds =
          {static_cast<float>(ScreenBounds.X),
           static_cast<float>(ScreenBounds.Y)},
      .position =
          {static_cast<float>(Position.X), static_cast<float>(Position.Y)},
      .scaleFactor = ScaleFactor,
      .button = RivePointerButtonToPointerButton(Button)};

  // Both questions answered in one round trip, since the callback already
  // runs on the server thread and nothing advances between the two calls.
  // Primary skips the query: it is the button a host gives Rive anyway, and
  // gating it would also drop presses on hover-only shapes, whose listeners
  // are bound to no button at all.
  const bool bQueryButton = Button != ERivePointerButton::Primary;
  const rive::StateMachineHandle Handle = NativeStateMachineHandle;
  return RunOnServerAndWait<rive::HitResult>(
             [Handle, Event, bQueryButton](rive::CommandServer* Server) {
               if (bQueryButton &&
                   !Server->listensToButtonAtSynchronized(Handle, Event))
               {
                 return rive::HitResult::none;
               }
               return Server->pointerDownSynchronized(Handle, Event);
             },
             rive::HitResult::none,
             TEXT("Pointer event")) != rive::HitResult::none;
}

bool FRiveStateMachine::PointerMove(
    const FGeometry& InGeometry,
    const FRiveDescriptor& InDescriptor,
    const FPointerEvent& InMouseEvent,
    float DPI)
{
  // Ignore when the mouse doesn't move.
  if (InMouseEvent.GetCursorDelta().GetAbsMax() <= 0)
  {
    return false;
  }
  bStateMachineSettled = false;

  FVector2D Position = USlateBlueprintLibrary::AbsoluteToLocal(
      InGeometry,
      InMouseEvent.GetScreenSpacePosition());
  FVector2D ScreenBounds = InGeometry.GetLocalSize();

  float ScaleFactor = 1.0f;
  if (InDescriptor.FitType == ERiveFitType::Layout)
  {
    ScaleFactor = InDescriptor.ScaleFactor;
    if (InDescriptor.bScaleLayoutByDPI)
      ScaleFactor *= DPI;
    Position *= DPI;
    ScreenBounds *= DPI;
  }

  FRiveRenderer* Renderer = IRiveRendererModule::Get().GetRenderer();
  check(Renderer);
  auto& CommandBuilder = Renderer->GetCommandBuilder();
  CommandBuilder.StateMachineMouseMove(
      NativeStateMachineHandle,
      {.fit = RiveFitTypeToFit(InDescriptor.FitType),
       .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
       .screenBounds =
           {static_cast<float>(ScreenBounds.X),
            static_cast<float>(ScreenBounds.Y)},
       .position =
           {static_cast<float>(Position.X), static_cast<float>(Position.Y)},
       .scaleFactor = ScaleFactor});

  return false;
}

bool FRiveStateMachine::PointerUp(
    const FGeometry& InGeometry,
    const FRiveDescriptor& InDescriptor,
    const FPointerEvent& InMouseEvent,
    float DPI,
    ERivePointerButton Button)
{
  bStateMachineSettled = false;
  FVector2D Position = USlateBlueprintLibrary::AbsoluteToLocal(
      InGeometry,
      InMouseEvent.GetScreenSpacePosition());
  FVector2D ScreenBounds = InGeometry.GetLocalSize();

  float ScaleFactor = 1.0f;
  if (InDescriptor.FitType == ERiveFitType::Layout)
  {
    ScaleFactor = InDescriptor.ScaleFactor;
    if (InDescriptor.bScaleLayoutByDPI)
      ScaleFactor *= DPI;
    Position *= DPI;
    ScreenBounds *= DPI;
  }

  return SendPointerEventAndWait(
             NativeStateMachineHandle,
             {.fit = RiveFitTypeToFit(InDescriptor.FitType),
              .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
              .screenBounds =
                  {static_cast<float>(ScreenBounds.X),
                   static_cast<float>(ScreenBounds.Y)},
              .position =
                  {static_cast<float>(Position.X),
                   static_cast<float>(Position.Y)},
              .scaleFactor = ScaleFactor,
              .button = RivePointerButtonToPointerButton(Button)},
             &rive::CommandServer::pointerUpSynchronized) !=
         rive::HitResult::none;
}

bool FRiveStateMachine::PointerExit(
    const FGeometry& InGeometry,
    const FRiveDescriptor& InDescriptor,
    const FPointerEvent& InMouseEvent,
    float DPI)
{
  bStateMachineSettled = false;
  float ScaleFactor = 1.0f;
  if (InDescriptor.FitType == ERiveFitType::Layout)
  {
    ScaleFactor = InDescriptor.ScaleFactor;
    if (InDescriptor.bScaleLayoutByDPI)
      ScaleFactor *= DPI;
  }

  FVector2D Position = USlateBlueprintLibrary::AbsoluteToLocal(
      InGeometry,
      InMouseEvent.GetScreenSpacePosition());
  FVector2D ScreenBounds = InGeometry.GetLocalSize();

  FRiveRenderer* Renderer = IRiveRendererModule::Get().GetRenderer();
  check(Renderer);
  auto& CommandBuilder = Renderer->GetCommandBuilder();
  CommandBuilder.StateMachineMouseOut(
      NativeStateMachineHandle,
      {.fit = RiveFitTypeToFit(InDescriptor.FitType),
       .alignment = RiveAlignementToAlignment(InDescriptor.Alignment),
       .screenBounds =
           {static_cast<float>(ScreenBounds.X),
            static_cast<float>(ScreenBounds.Y)},
       .position =
           {static_cast<float>(Position.X), static_cast<float>(Position.Y)},
       .scaleFactor = ScaleFactor});

  return false;
}

// Function-local static because EKeys are engine globals built during module
// startup: a namespace-scope table would race their initialisation. Built once,
// on the first keystroke of the session.
static const TMap<FKey, rive::Key>& RiveKeyTable()
{
  static const TMap<FKey, rive::Key> Table = {
      {EKeys::A, rive::Key::a},
      {EKeys::B, rive::Key::b},
      {EKeys::C, rive::Key::c},
      {EKeys::D, rive::Key::d},
      {EKeys::E, rive::Key::e},
      {EKeys::F, rive::Key::f},
      {EKeys::G, rive::Key::g},
      {EKeys::H, rive::Key::h},
      {EKeys::I, rive::Key::i},
      {EKeys::J, rive::Key::j},
      {EKeys::K, rive::Key::k},
      {EKeys::L, rive::Key::l},
      {EKeys::M, rive::Key::m},
      {EKeys::N, rive::Key::n},
      {EKeys::O, rive::Key::o},
      {EKeys::P, rive::Key::p},
      {EKeys::Q, rive::Key::q},
      {EKeys::R, rive::Key::r},
      {EKeys::S, rive::Key::s},
      {EKeys::T, rive::Key::t},
      {EKeys::U, rive::Key::u},
      {EKeys::V, rive::Key::v},
      {EKeys::W, rive::Key::w},
      {EKeys::X, rive::Key::x},
      {EKeys::Y, rive::Key::y},
      {EKeys::Z, rive::Key::z},

      {EKeys::Zero, rive::Key::key0},
      {EKeys::One, rive::Key::key1},
      {EKeys::Two, rive::Key::key2},
      {EKeys::Three, rive::Key::key3},
      {EKeys::Four, rive::Key::key4},
      {EKeys::Five, rive::Key::key5},
      {EKeys::Six, rive::Key::key6},
      {EKeys::Seven, rive::Key::key7},
      {EKeys::Eight, rive::Key::key8},
      {EKeys::Nine, rive::Key::key9},

      {EKeys::NumPadZero, rive::Key::kp0},
      {EKeys::NumPadOne, rive::Key::kp1},
      {EKeys::NumPadTwo, rive::Key::kp2},
      {EKeys::NumPadThree, rive::Key::kp3},
      {EKeys::NumPadFour, rive::Key::kp4},
      {EKeys::NumPadFive, rive::Key::kp5},
      {EKeys::NumPadSix, rive::Key::kp6},
      {EKeys::NumPadSeven, rive::Key::kp7},
      {EKeys::NumPadEight, rive::Key::kp8},
      {EKeys::NumPadNine, rive::Key::kp9},
      {EKeys::Decimal, rive::Key::kpDecimal},
      {EKeys::Divide, rive::Key::kpDivide},
      {EKeys::Multiply, rive::Key::kpMultiply},
      {EKeys::Subtract, rive::Key::kpSubtract},
      {EKeys::Add, rive::Key::kpAdd},

      {EKeys::SpaceBar, rive::Key::space},
      {EKeys::Enter, rive::Key::enter},
      {EKeys::BackSpace, rive::Key::backspace},
      {EKeys::Tab, rive::Key::tab},
      {EKeys::Escape, rive::Key::escape},
      {EKeys::Insert, rive::Key::insert},
      {EKeys::Delete, rive::Key::deleteKey},
      {EKeys::Left, rive::Key::left},
      {EKeys::Right, rive::Key::right},
      {EKeys::Up, rive::Key::up},
      {EKeys::Down, rive::Key::down},
      {EKeys::Home, rive::Key::home},
      {EKeys::End, rive::Key::end},
      {EKeys::PageUp, rive::Key::pageUp},
      {EKeys::PageDown, rive::Key::pageDown},

      {EKeys::Apostrophe, rive::Key::apostrophe},
      {EKeys::Comma, rive::Key::comma},
      {EKeys::Hyphen, rive::Key::minus},
      {EKeys::Period, rive::Key::period},
      {EKeys::Slash, rive::Key::slash},
      {EKeys::Semicolon, rive::Key::semicolon},
      {EKeys::Equals, rive::Key::equal},
      {EKeys::LeftBracket, rive::Key::leftBracket},
      {EKeys::Backslash, rive::Key::backslash},
      {EKeys::RightBracket, rive::Key::rightBracket},
      {EKeys::Tilde, rive::Key::graveAccent},

      {EKeys::CapsLock, rive::Key::capsLock},
      {EKeys::ScrollLock, rive::Key::scrollLock},
      {EKeys::NumLock, rive::Key::numLock},
      {EKeys::Pause, rive::Key::pause},

      {EKeys::F1, rive::Key::f1},
      {EKeys::F2, rive::Key::f2},
      {EKeys::F3, rive::Key::f3},
      {EKeys::F4, rive::Key::f4},
      {EKeys::F5, rive::Key::f5},
      {EKeys::F6, rive::Key::f6},
      {EKeys::F7, rive::Key::f7},
      {EKeys::F8, rive::Key::f8},
      {EKeys::F9, rive::Key::f9},
      {EKeys::F10, rive::Key::f10},
      {EKeys::F11, rive::Key::f11},
      {EKeys::F12, rive::Key::f12},

      // Sent as keys in their own right as well as modifier state, so a
      // listener watching for Shift being held gets the press.
      {EKeys::LeftShift, rive::Key::leftShift},
      {EKeys::RightShift, rive::Key::rightShift},
      {EKeys::LeftControl, rive::Key::leftControl},
      {EKeys::RightControl, rive::Key::rightControl},
      {EKeys::LeftAlt, rive::Key::leftAlt},
      {EKeys::RightAlt, rive::Key::rightAlt},
      {EKeys::LeftCommand, rive::Key::leftSuper},
      {EKeys::RightCommand, rive::Key::rightSuper},
  };

  return Table;
}

static rive::KeyModifiers RiveModifiers(const FModifierKeysState& Modifiers)
{
  rive::KeyModifiers Result = rive::KeyModifiers::none;

  if (Modifiers.IsShiftDown())
    Result = Result | rive::KeyModifiers::shift;
  if (Modifiers.IsControlDown())
    Result = Result | rive::KeyModifiers::ctrl;
  if (Modifiers.IsAltDown())
    Result = Result | rive::KeyModifiers::alt;
  if (Modifiers.IsCommandDown())
    Result = Result | rive::KeyModifiers::meta;

  return Result;
}

bool FRiveStateMachine::KeyInput(const FKeyEvent& InKeyEvent, bool bPressed)
{
  // The event's own modifiers rather than the keyboard's current state, so a
  // queued event is answered with the world as it was when it happened.
  return KeyInput(
      InKeyEvent.GetKey(),
      FModifierKeysState(
          InKeyEvent.IsShiftDown(),
          InKeyEvent.IsShiftDown(),
          InKeyEvent.IsControlDown(),
          InKeyEvent.IsControlDown(),
          InKeyEvent.IsAltDown(),
          InKeyEvent.IsAltDown(),
          InKeyEvent.IsCommandDown(),
          InKeyEvent.IsCommandDown(),
          InKeyEvent.AreCapsLocked()),
      bPressed,
      InKeyEvent.IsRepeat());
}

bool FRiveStateMachine::KeyInput(
    FKey InKey,
    FModifierKeysState InModifiers,
    bool bPressed,
    bool bRepeat)
{
  // Missing before anything is sent, so an unmapped key never blocks.
  const rive::Key* Key = RiveKeyTable().Find(InKey);
  if (Key == nullptr)
    return false;

  // A settled artboard will not redraw, so a caret would move in the data and
  // never on screen.
  bStateMachineSettled = false;

  const rive::Key NativeKey = *Key;
  const rive::KeyModifiers Modifiers = RiveModifiers(InModifiers);
  const rive::StateMachineHandle Handle = NativeStateMachineHandle;

  return RunOnServerAndWait<bool>(
      [Handle, NativeKey, Modifiers, bPressed, bRepeat](
          rive::CommandServer* Server) {
        rive::StateMachineInstance* Instance = Server->getStateMachineInstance(
            Handle);
        return Instance != nullptr
                   ? Instance->keyInput(NativeKey, Modifiers, bPressed, bRepeat)
                   : false;
      },
      false,
      TEXT("Key event"));
}

bool FRiveStateMachine::TextInput(const FString& InText)
{
  if (InText.IsEmpty())
    return false;

  bStateMachineSettled = false;

  const std::string Text(TCHAR_TO_UTF8(*InText));
  const rive::StateMachineHandle Handle = NativeStateMachineHandle;

  return RunOnServerAndWait<bool>(
      [Handle, Text](rive::CommandServer* Server) {
        rive::StateMachineInstance* Instance = Server->getStateMachineInstance(
            Handle);
        return Instance != nullptr ? Instance->textInput(Text) : false;
      },
      false,
      TEXT("Text input"));
}

void FRiveStateMachine::ClearFocus()
{
  bStateMachineSettled = false;

  // Nothing waits on losing focus, so this one does not block. Immediate
  // rather than end of frame all the same: a focus query or entry sent later
  // in the frame would otherwise run first and be undone by this clear.
  const rive::StateMachineHandle Handle = NativeStateMachineHandle;
  IRiveRendererModule::GetCommandBuilder().RunOnceImmediate(
      [Handle, Cache = CachedHasFocus](rive::CommandServer* Server) {
        rive::StateMachineInstance* Instance = Server->getStateMachineInstance(
            Handle);
        if (Instance != nullptr)
        {
          Instance->clearFocus();
          Cache->store(false);
        }
      });
}

static bool IsRiveFocusMove(EUINavigation Navigation)
{
  switch (Navigation)
  {
    case EUINavigation::Left:
    case EUINavigation::Right:
    case EUINavigation::Up:
    case EUINavigation::Down:
    case EUINavigation::Next:
    case EUINavigation::Previous:
      return true;
    default:
      return false;
  }
}

// Runs on the command server.
static bool MoveFocusOnServer(
    rive::StateMachineInstance* Instance,
    EUINavigation Navigation)
{
  if (Instance == nullptr || !Instance->focusState().hasFocus)
    return false;
  switch (Navigation)
  {
    case EUINavigation::Left:
      return Instance->focusLeft();
    case EUINavigation::Right:
      return Instance->focusRight();
    case EUINavigation::Up:
      return Instance->focusUp();
    case EUINavigation::Down:
      return Instance->focusDown();
    case EUINavigation::Next:
    case EUINavigation::Previous:
      break;
    default:
      return false;
  }

  // Running off either end of the tab order clears focus. Put the node back
  // in case Slate has nowhere to send focus either. See "Tab running off the
  // end" in the plugin docs.
  rive::FocusManager* Manager = Instance->focusManager();
  rive::rcp<rive::FocusNode> Previous;
  if (Manager != nullptr)
  {
    Previous = Manager->primaryFocus();
  }
  const bool bMoved = Navigation == EUINavigation::Next
                          ? Instance->focusNext()
                          : Instance->focusPrevious();
  if (!bMoved && Previous && !Instance->focusState().hasFocus)
  {
    Manager->setFocus(Previous);
  }
  return bMoved;
}

bool FRiveStateMachine::MoveFocus(
    EUINavigation Navigation,
    TFunction<void(bool bMoved)> OnResult)
{
  if (!IsRiveFocusMove(Navigation))
    return false;

  // The focus and blur listeners a move fires run on the next advance.
  bStateMachineSettled = false;

  // Immediate, so a d-pad listener on the focused node hears the press
  // before focus moves away from it. The answer is posted back rather than
  // waited for; see "Navigation" in the plugin docs.
  const rive::StateMachineHandle Handle = NativeStateMachineHandle;
  IRiveRendererModule::GetCommandBuilder().RunOnceImmediate(
      [Handle,
       Navigation,
       Cache = CachedHasFocus,
       OnResult = MoveTemp(OnResult)](rive::CommandServer* Server) {
        rive::StateMachineInstance* Instance = Server->getStateMachineInstance(
            Handle);
        const bool bMoved = MoveFocusOnServer(Instance, Navigation);
        if (Instance != nullptr)
        {
          Cache->store(Instance->focusState().hasFocus);
        }
        // Posted even when the instance is gone, so a widget waiting on the
        // answer still gets to leave.
        AsyncTask(ENamedThreads::GameThread, [OnResult, bMoved]() {
          OnResult(bMoved);
        });
      });
  return true;
}

// TODO: stand-in for focus entry until the directional focus proposal lands.
// See "Future work" in the plugin docs.
void FRiveStateMachine::FocusFirst()
{
  // The focus listeners it fires run on the next advance.
  bStateMachineSettled = false;

  const rive::StateMachineHandle Handle = NativeStateMachineHandle;
  IRiveRendererModule::GetCommandBuilder().RunOnceImmediate(
      [Handle, Cache = CachedHasFocus](rive::CommandServer* Server) {
        rive::StateMachineInstance* Instance = Server->getStateMachineInstance(
            Handle);
        if (Instance != nullptr && !Instance->focusState().hasFocus)
        {
          Instance->focusNext();
          Cache->store(Instance->focusState().hasFocus);
        }
      });
}

namespace
{
constexpr int32 GamepadButtonCount = 17;
constexpr int32 GamepadAxisCount = 6;
constexpr int32 NoSlot = -1;

// Where an Unreal gamepad key lands in the W3C standard layout. Triggers are
// both a button with analog pressure and an axis there, so they fill both.
struct FGamepadSlot
{
  int32 Button = NoSlot;
  int32 Axis = NoSlot;
  // Unreal's sticks read positive up, the W3C layout's positive down.
  bool bInvert = false;
};

using rive::StandardGamepadAxis;
using rive::StandardGamepadButton;

constexpr int32 Slot(StandardGamepadButton Button)
{
  return static_cast<int32>(Button);
}

constexpr int32 Slot(StandardGamepadAxis Axis)
{
  return static_cast<int32>(Axis);
}

const TMap<FKey, FGamepadSlot>& GamepadSlotTable()
{
  // No entry for the digital trigger keys: the trigger axes carry the same
  // press with its pressure, and sending both would report it twice. Unreal
  // has no standard key for the home button (W3C 16).
  static const TMap<FKey, FGamepadSlot> Table = {
      {EKeys::Gamepad_FaceButton_Bottom, {Slot(StandardGamepadButton::south)}},
      {EKeys::Gamepad_FaceButton_Right, {Slot(StandardGamepadButton::east)}},
      {EKeys::Gamepad_FaceButton_Left, {Slot(StandardGamepadButton::west)}},
      {EKeys::Gamepad_FaceButton_Top, {Slot(StandardGamepadButton::north)}},
      {EKeys::Gamepad_LeftShoulder,
       {Slot(StandardGamepadButton::leftShoulder)}},
      {EKeys::Gamepad_RightShoulder,
       {Slot(StandardGamepadButton::rightShoulder)}},
      {EKeys::Gamepad_Special_Left, {Slot(StandardGamepadButton::back)}},
      {EKeys::Gamepad_Special_Right, {Slot(StandardGamepadButton::forward)}},
      {EKeys::Gamepad_LeftThumbstick, {Slot(StandardGamepadButton::leftStick)}},
      {EKeys::Gamepad_RightThumbstick,
       {Slot(StandardGamepadButton::rightStick)}},
      {EKeys::Gamepad_DPad_Up, {Slot(StandardGamepadButton::dpadUp)}},
      {EKeys::Gamepad_DPad_Down, {Slot(StandardGamepadButton::dpadDown)}},
      {EKeys::Gamepad_DPad_Left, {Slot(StandardGamepadButton::dpadLeft)}},
      {EKeys::Gamepad_DPad_Right, {Slot(StandardGamepadButton::dpadRight)}},

      {EKeys::Gamepad_LeftX, {NoSlot, Slot(StandardGamepadAxis::leftX)}},
      {EKeys::Gamepad_LeftY, {NoSlot, Slot(StandardGamepadAxis::leftY), true}},
      {EKeys::Gamepad_RightX, {NoSlot, Slot(StandardGamepadAxis::rightX)}},
      {EKeys::Gamepad_RightY,
       {NoSlot, Slot(StandardGamepadAxis::rightY), true}},
      {EKeys::Gamepad_LeftTriggerAxis,
       {Slot(StandardGamepadButton::leftTrigger),
        Slot(StandardGamepadAxis::leftTrigger)}},
      {EKeys::Gamepad_RightTriggerAxis,
       {Slot(StandardGamepadButton::rightTrigger),
        Slot(StandardGamepadAxis::rightTrigger)}},
  };
  return Table;
}

// The batch wire format is little endian, fixed width.
void AppendU8(TArray<uint8>& Buffer, uint8 Value) { Buffer.Add(Value); }

void AppendU32(TArray<uint8>& Buffer, uint32 Value)
{
  for (int32 Shift = 0; Shift < 32; Shift += 8)
  {
    Buffer.Add(static_cast<uint8>((Value >> Shift) & 0xFF));
  }
}

void AppendF32(TArray<uint8>& Buffer, float Value)
{
  uint32 Bits;
  FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
  AppendU32(Buffer, Bits);
}

// Kind (0 button, 1 axis), index and value of each field an update moves.
using FGamepadChanges = TArray<
    TTuple<uint8, uint8, float>,
    TInlineAllocator<4>>;

void AppendUpdate(
    TArray<uint8>& Buffer,
    int32 DeviceId,
    const FGamepadChanges& Changes)
{
  if (Changes.IsEmpty())
    return;

  AppendU8(Buffer, static_cast<uint8>(rive::GamepadRecordType::update));
  AppendU32(Buffer, static_cast<uint32>(DeviceId));
  AppendU8(Buffer, static_cast<uint8>(Changes.Num()));
  for (const auto& Change : Changes)
  {
    AppendU8(Buffer, Change.Get<0>());
    AppendU8(Buffer, Change.Get<1>());
    AppendF32(Buffer, Change.Get<2>());
  }
}

void SubmitGamepadBatch(rive::StateMachineHandle Handle, TArray<uint8> Buffer)
{
  // Immediate rather than end of frame, so a focus query that follows sees
  // this batch already dispatched.
  IRiveRendererModule::GetCommandBuilder().RunOnceImmediate(
      [Handle, Buffer = MoveTemp(Buffer)](rive::CommandServer* Server) {
        rive::StateMachineInstance* Instance = Server->getStateMachineInstance(
            Handle);
        if (Instance != nullptr)
        {
          Instance->submitGamepadsFromBuffer(Buffer.GetData(), Buffer.Num());
        }
      });
}
} // namespace

bool FRiveStateMachine::GamepadInput(
    const FKey& InKey,
    float Value,
    int32 DeviceId)
{
  const FGamepadSlot* Found = GamepadSlotTable().Find(InKey);
  if (Found == nullptr)
    return false;

  const FGamepadSlot GamepadSlot = *Found;
  const float SlotValue = GamepadSlot.bInvert ? -Value : Value;

  TArray<uint8> Buffer;
  AppendU32(Buffer, rive::kGamepadBatchWireVersion);

  // The runtime drops updates from a pad it has not been told about, so the
  // first event from one announces it at rest.
  FGamepadState* State = Gamepads.Find(DeviceId);
  if (State == nullptr)
  {
    State = &Gamepads.Add(DeviceId);
    State->Buttons.Init(0.f, GamepadButtonCount);
    State->Axes.Init(0.f, GamepadAxisCount);

    AppendU8(Buffer, static_cast<uint8>(rive::GamepadRecordType::connected));
    AppendU32(Buffer, static_cast<uint32>(DeviceId));
    AppendU8(Buffer, 0); // standard mapping
    AppendU8(Buffer, GamepadButtonCount);
    AppendU8(Buffer, GamepadAxisCount);
    AppendU8(Buffer, 0); // padding
    for (float Button : State->Buttons)
      AppendF32(Buffer, Button);
    for (float Axis : State->Axes)
      AppendF32(Buffer, Axis);
  }

  FGamepadChanges Changes;
  if (GamepadSlot.Button != NoSlot &&
      State->Buttons[GamepadSlot.Button] != SlotValue)
  {
    State->Buttons[GamepadSlot.Button] = SlotValue;
    Changes.Emplace(0, static_cast<uint8>(GamepadSlot.Button), SlotValue);
  }
  if (GamepadSlot.Axis != NoSlot && State->Axes[GamepadSlot.Axis] != SlotValue)
  {
    State->Axes[GamepadSlot.Axis] = SlotValue;
    Changes.Emplace(1, static_cast<uint8>(GamepadSlot.Axis), SlotValue);
  }

  AppendUpdate(Buffer, DeviceId, Changes);

  // Only the version: a repeat of a value already sent.
  if (Buffer.Num() == sizeof(uint32))
    return true;

  bStateMachineSettled = false;
  SubmitGamepadBatch(NativeStateMachineHandle, MoveTemp(Buffer));
  return true;
}

void FRiveStateMachine::ReleaseGamepads()
{
  TArray<uint8> Buffer;
  AppendU32(Buffer, rive::kGamepadBatchWireVersion);

  for (auto& [DeviceId, State] : Gamepads)
  {
    FGamepadChanges Changes;
    for (int32 Index = 0; Index < State.Buttons.Num(); ++Index)
    {
      if (State.Buttons[Index] != 0.f)
      {
        State.Buttons[Index] = 0.f;
        Changes.Emplace(0, static_cast<uint8>(Index), 0.f);
      }
    }
    for (int32 Index = 0; Index < State.Axes.Num(); ++Index)
    {
      if (State.Axes[Index] != 0.f)
      {
        State.Axes[Index] = 0.f;
        Changes.Emplace(1, static_cast<uint8>(Index), 0.f);
      }
    }
    AppendUpdate(Buffer, DeviceId, Changes);
  }

  // Nothing was held.
  if (Buffer.Num() == sizeof(uint32))
    return;

  bStateMachineSettled = false;
  SubmitGamepadBatch(NativeStateMachineHandle, MoveTemp(Buffer));
}

void FRiveStateMachine::BindViewModel(TObjectPtr<URiveViewModel> ViewModel)
{
  bStateMachineSettled = false;
  if (!::IsValid(ViewModel))
  {
    UE_LOG(
        LogRive,
        Error,
        TEXT("FRiveStateMachine::BindViewModel ViewModel was invalid"));
    return;
  }
  FRiveRenderer* Renderer = IRiveRendererModule::Get().GetRenderer();
  check(Renderer);
  auto& CommandBuilder = Renderer->GetCommandBuilder();
  CommandBuilder.StateMachineBindViewModel(
      NativeStateMachineHandle,
      ViewModel->GetNativeHandle());
}

void FRiveStateMachine::SetStateMachineSettled(bool inStateMachineSettled)
{
  UE_LOG(
      LogRive,
      VeryVerbose,
      TEXT("Rive StateMachine %s SetSettled %s"),
      *StateMachineName,
      inStateMachineSettled ? TEXT("True") : TEXT("False"));
  bStateMachineSettled = inStateMachineSettled;
}

void FRiveStateMachine::OnStateMachineError(
    uint64_t requestId,
    std::string error)
{}

#endif // WITH_RIVE
