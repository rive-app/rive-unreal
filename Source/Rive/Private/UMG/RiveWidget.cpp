// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "UMG/RiveWidget.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveAudioEngine.h"
#include "Rive/RiveArtboard.h"
#include "TimerManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/PanelWidget.h"
#include "Slate/SRiveLeafWidget.h"

#define LOCTEXT_NAMESPACE "RiveWidget"

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

FReply URiveWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                            const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return FReply::Unhandled();
    }

    if (!IsValid(RiveArtboard))
    {
        return FReply::Unhandled();
    }

    if (!RiveArtboard->PointerDown(
            InGeometry,
            RiveDescriptor,
            InMouseEvent,
            UWidgetLayoutLibrary::GetViewportScale(this)))
    {
        return FReply::Unhandled();
    }

    // Only on a press that hit something, so clicking through a transparent
    // artboard leaves focus where the player put it.
    return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}

FReply URiveWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry,
                                          const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return FReply::Unhandled();
    }

    if (!IsValid(RiveArtboard))
    {
        return FReply::Unhandled();
    }

    return RiveArtboard->PointerUp(InGeometry,
                                   RiveDescriptor,
                                   InMouseEvent,
                                   UWidgetLayoutLibrary::GetViewportScale(this))
               ? FReply::Handled()
               : FReply::Unhandled();
}

FReply URiveWidget::NativeOnMouseMove(const FGeometry& InGeometry,
                                      const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseMove(InGeometry, InMouseEvent);

    if (IsValid(RiveArtboard))
    {
        RiveArtboard->PointerMove(InGeometry,
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
        RiveArtboard->PointerExit(GetCachedGeometry(),
                                  RiveDescriptor,
                                  InMouseEvent,
                                  UWidgetLayoutLibrary::GetViewportScale(this));
    }
}

FReply URiveWidget::NativeOnTouchStarted(const FGeometry& InGeometry,
                                         const FPointerEvent& InGestureEvent)
{
    Super::NativeOnTouchStarted(InGeometry, InGestureEvent);

    if (IsValid(RiveArtboard))
    {
        RiveArtboard->PointerDown(GetCachedGeometry(),
                                  RiveDescriptor,
                                  InGestureEvent,
                                  UWidgetLayoutLibrary::GetViewportScale(this));
    }

    return FReply::Handled();
}

FReply URiveWidget::NativeOnTouchMoved(const FGeometry& InGeometry,
                                       const FPointerEvent& InGestureEvent)
{
    Super::NativeOnTouchMoved(InGeometry, InGestureEvent);

    if (IsValid(RiveArtboard))
    {
        RiveArtboard->PointerMove(GetCachedGeometry(),
                                  RiveDescriptor,
                                  InGestureEvent,
                                  UWidgetLayoutLibrary::GetViewportScale(this));
    }

    return FReply::Handled();
}

FReply URiveWidget::NativeOnTouchEnded(const FGeometry& InGeometry,
                                       const FPointerEvent& InGestureEvent)
{
    Super::NativeOnTouchEnded(InGeometry, InGestureEvent);

    if (IsValid(RiveArtboard))
    {
        RiveArtboard->PointerUp(GetCachedGeometry(),
                                RiveDescriptor,
                                InGestureEvent,
                                UWidgetLayoutLibrary::GetViewportScale(this));
    }

    return FReply::Handled();
}

FReply URiveWidget::NativeOnKeyDown(const FGeometry& InGeometry,
                                    const FKeyEvent& InKeyEvent)
{
    if (!IsValid(RiveArtboard))
    {
        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
    }

    // Handled only when the runtime wanted it, so Escape still closes the
    // screen when nothing in the artboard is focused.
    if (RiveArtboard->KeyInput(InKeyEvent, true))
    {
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply URiveWidget::NativeOnKeyUp(const FGeometry& InGeometry,
                                  const FKeyEvent& InKeyEvent)
{
    if (!IsValid(RiveArtboard))
    {
        return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
    }

    if (RiveArtboard->KeyInput(InKeyEvent, false))
    {
        return FReply::Handled();
    }

    return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FReply URiveWidget::NativeOnKeyChar(const FGeometry& InGeometry,
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

void URiveWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
    Super::NativeOnFocusLost(InFocusEvent);

    if (IsValid(RiveArtboard))
    {
        // Otherwise a text input keeps its caret blinking after the player has
        // clicked away.
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
        RiveWidget->SetArtboard(RiveArtboard);
        if (IsValid(RiveAudioEngine))
        {
            RiveArtboard->SetAudioEngine(RiveAudioEngine);
        }
    }
}

URiveArtboard* URiveWidget::GetArtboard() const { return RiveArtboard; }

#if WITH_EDITOR
void URiveWidget::PostEditChangeChainProperty(
    FPropertyChangedChainEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    const FName ActiveMemberNodeName =
        *PropertyChangedEvent.PropertyChain.GetActiveMemberNode()
             ->GetValue()
             ->GetName();

    if (PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, RiveFile) ||
        PropertyName ==
            GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardName) ||
        PropertyName ==
            GET_MEMBER_NAME_CHECKED(FRiveDescriptor, StateMachineName) ||
        PropertyName ==
            GET_MEMBER_NAME_CHECKED(FRiveDescriptor, bAutoBindDefaultViewModel))
    {
        TArray<FString> ArtboardNames;
        if (RiveDescriptor.RiveFile)
        {
            for (const auto& Artboard :
                 RiveDescriptor.RiveFile->ArtboardDefinitions)
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
            for (const auto& Artboard :
                 RiveDescriptor.RiveFile->ArtboardDefinitions)
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
        else if (RiveDescriptor.StateMachineName.IsEmpty() ||
                 (RiveDescriptor.StateMachineName != TEXT("None") &&
                  !StateMachineNames.Contains(RiveDescriptor.StateMachineName)))
        {
            RiveDescriptor.StateMachineName = StateMachineNames[0];
        }

        if (IsValid(RiveDescriptor.RiveFile))
        {
            if (RiveDescriptor.ArtboardName.IsEmpty())
            {
                UE_LOG(LogRive,
                       Error,
                       TEXT("URiveWidget::PostEditChangeChainProperty Selected "
                            "artboard is empty"));
                return;
            }

            RiveArtboard =
                URiveFile::MakeArtboardFromDescriptor(RiveDescriptor);

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

    if (!RiveWidget.IsValid())
    {
        return;
    }

    if (RiveArtboard == nullptr && RiveDescriptor.RiveFile != nullptr &&
        IsValid(RiveDescriptor.RiveFile))
    {
        if (RiveDescriptor.ArtboardName.IsEmpty())
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("URiveWidget::Setup Selected artboard is empty"));
            return;
        }

        RiveArtboard = URiveFile::MakeArtboardFromDescriptor(RiveDescriptor);
    }

    if (IsValid(RiveArtboard))
    {
        RiveDescriptor.ArtboardName = RiveArtboard->GetArtboardName();
        RiveWidget->SetArtboard(RiveArtboard);
        RiveWidget->SetRiveDescriptor(RiveDescriptor);
        if (IsValid(RiveAudioEngine))
        {
            RiveArtboard->SetAudioEngine(RiveAudioEngine);
        }
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
