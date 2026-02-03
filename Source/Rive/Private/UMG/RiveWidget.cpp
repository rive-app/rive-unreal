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

    Setup();

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

    if (IsValid(RiveArtboard))
    {
        RiveArtboard->PointerDown(InGeometry,
                                  RiveDescriptor,
                                  InMouseEvent,
                                  UWidgetLayoutLibrary::GetViewportScale(this));
    }

    return FReply::Handled();
}

FReply URiveWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry,
                                          const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return FReply::Unhandled();
    }

    if (IsValid(RiveArtboard))
    {
        RiveArtboard->PointerUp(InGeometry,
                                RiveDescriptor,
                                InMouseEvent,
                                UWidgetLayoutLibrary::GetViewportScale(this));
    }

    return FReply::Handled();
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

TArray<FString> URiveWidget::GetArtboardNamesForDropdown() const
{
    TArray<FString> Output;

    if (RiveDescriptor.RiveFile)
    {
        for (auto Artboard : RiveDescriptor.RiveFile->ArtboardDefinitions)
        {
            Output.Add(Artboard.Name);
        }
    }

    return Output;
}

TArray<FString> URiveWidget::GetStateMachineNamesForDropdown() const
{
    if (RiveDescriptor.RiveFile)
    {
        for (auto Artboard : RiveDescriptor.RiveFile->ArtboardDefinitions)
        {
            if (Artboard.Name.Equals(RiveDescriptor.ArtboardName))
            {
                return Artboard.StateMachineNames;
            }
        }
    }

    return {};
}

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
            GET_MEMBER_NAME_CHECKED(FRiveDescriptor, bAutoBindDefaultViewModel))
    {
        TArray<FString> ArtboardNames = GetArtboardNamesForDropdown();
        if ((RiveDescriptor.ArtboardName.IsEmpty() ||
             !ArtboardNames.Contains(RiveDescriptor.ArtboardName) &&
                 !ArtboardNames.IsEmpty()))
        {
            RiveDescriptor.ArtboardName = ArtboardNames[0];
        }

        TArray<FString> StateMachineNames = GetStateMachineNamesForDropdown();
        if (StateMachineNames.Num() == 1)
        {
            RiveDescriptor.StateMachineName = StateMachineNames[0];
        }
        else if (RiveDescriptor.StateMachineName.IsEmpty() ||
                 !StateMachineNames.Contains(RiveDescriptor.StateMachineName) &&
                     !StateMachineNames.IsEmpty())
        {
            RiveDescriptor.StateMachineName = StateMachineNames[0];
        }
        else
        {
            RiveDescriptor.StateMachineName = "None";
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

            RiveArtboard = RiveDescriptor.RiveFile->CreateArtboardNamed(
                RiveDescriptor.ArtboardName,
                RiveDescriptor.bAutoBindDefaultViewModel);

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

        RiveArtboard = RiveDescriptor.RiveFile->CreateArtboardNamed(
            RiveDescriptor.ArtboardName,
            RiveDescriptor.bAutoBindDefaultViewModel);
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

    Setup();
}
#undef LOCTEXT_NAMESPACE
