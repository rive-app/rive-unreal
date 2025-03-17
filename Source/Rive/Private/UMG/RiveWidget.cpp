// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveWidget.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveTextureObject.h"
#include "Slate/SRiveWidget.h"
#include "TimerManager.h"
#include "Components/PanelWidget.h"

#define LOCTEXT_NAMESPACE "RiveWidget"
namespace UE::Private::RiveWidget
{
FBox2f CalculateRenderTextureExtentsInViewport(const FVector2f& InTextureSize,
                                               const FVector2f& InViewportSize);
FVector2f CalculateLocalPointerCoordinatesFromViewport(
    URiveTexture* InRiveTexture,
    URiveArtboard* InArtboard,
    const FGeometry& MyGeometry,
    const FPointerEvent& MouseEvent);

FVector2f GetInputCoordinates(URiveTexture* InRiveTexture,
                              URiveArtboard* InRiveArtboard,
                              const FGeometry& MyGeometry,
                              const FPointerEvent& MouseEvent,
                              const float InScaleFactor = 1.0f)
{
    // Convert absolute input position to viewport local position
    FDeprecateSlateVector2D LocalPosition =
        MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

    if (InScaleFactor != -1)
        LocalPosition /= InScaleFactor;

    // Because our RiveTexture can be a different pixel size than our viewport,
    // we have to scale the x,y coords
    const FVector2f ViewportSize = MyGeometry.GetLocalSize();
    const FBox2f TextureBox =
        CalculateRenderTextureExtentsInViewport(InRiveTexture->Size,
                                                ViewportSize);
    return InRiveTexture->GetLocalCoordinatesFromExtents(InRiveArtboard,
                                                         LocalPosition,
                                                         TextureBox);
}

FBox2f CalculateRenderTextureExtentsInViewport(const FVector2f& InTextureSize,
                                               const FVector2f& InViewportSize)
{
    const float TextureAspectRatio = InTextureSize.X / InTextureSize.Y;
    const float ViewportAspectRatio = InViewportSize.X / InViewportSize.Y;

    if (ViewportAspectRatio >
        TextureAspectRatio) // Viewport wider than the Texture => height should
                            // be the same
    {
        FVector2f Size{InViewportSize.Y * TextureAspectRatio, InViewportSize.Y};
        float XOffset = (InViewportSize.X - Size.X) * 0.5f;
        return {{XOffset, 0}, {XOffset + Size.X, Size.Y}};
    }
    else // Viewport taller than the Texture => width should be the same
    {
        FVector2f Size{(float)InViewportSize.X,
                       InViewportSize.X / TextureAspectRatio};
        float YOffset = (InViewportSize.Y - Size.Y) * 0.5f;
        return {{0, YOffset}, {Size.X, YOffset + Size.Y}};
    }
}

FVector2f CalculateLocalPointerCoordinatesFromViewport(
    URiveTexture* InRiveTexture,
    URiveArtboard* InArtboard,
    const FGeometry& MyGeometry,
    const FPointerEvent& MouseEvent)
{
    const FVector2f MouseLocal =
        MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    const FVector2f ViewportSize = MyGeometry.GetLocalSize();
    const FBox2f TextureBox =
        CalculateRenderTextureExtentsInViewport(InRiveTexture->Size,
                                                ViewportSize);
    return InRiveTexture->GetLocalCoordinatesFromExtents(InArtboard,
                                                         MouseLocal,
                                                         TextureBox);
}
} // namespace UE::Private::RiveWidget

URiveWidget::~URiveWidget()
{
    if (RiveWidget != nullptr)
    {
        RiveWidget->SetRiveTexture(nullptr);
    }

    RiveWidget.Reset();

    if (RiveTextureObject != nullptr)
    {
        RiveTextureObject->MarkAsGarbage();
        RiveTextureObject = nullptr;
    }
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

    if (RiveWidget != nullptr)
    {
        RiveWidget->SetRiveTexture(nullptr);
    }

    RiveWidget.Reset();

    if (RiveTextureObject != nullptr)
    {
        RiveTextureObject->MarkAsGarbage();
        RiveTextureObject = nullptr;
    }
}

TSharedRef<SWidget> URiveWidget::RebuildWidget()
{
    RiveWidget =
        SNew(SRiveWidget)
            .OnSizeChanged(BIND_UOBJECT_DELEGATE(SRiveWidget::FOnSizeChanged,
                                                 OnSWidgetSizeChanged));

    if (!RiveTextureObject && RiveWidget.IsValid())
    {
        RiveTextureObject = NewObject<URiveTextureObject>();
        RiveTextureObject->Size =
            FIntPoint::ZeroValue; // Setting to zero value here will make the
                                  // rive texture use the artboard size
                                  // initially

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(TimerHandle);
            World->GetTimerManager().SetTimer(
                TimerHandle,
                [this]() { Setup(); },
                0.05f,
                false);
        }
    }

    return RiveWidget.ToSharedRef();
}

FReply URiveWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                            const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return FReply::Unhandled();
    }

    return OnInput(InGeometry,
                   InMouseEvent,
                   [this](const FVector2f& InputCoordinates,
                          FRiveStateMachine* InStateMachine) {
                       if (InStateMachine)
                       {
                           return InStateMachine->PointerDown(InputCoordinates);
                       }
                       return false;
                   });
}

FReply URiveWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry,
                                          const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return FReply::Unhandled();
    }

    return OnInput(InGeometry,
                   InMouseEvent,
                   [this](const FVector2f& InputCoordinates,
                          FRiveStateMachine* InStateMachine) {
                       if (InStateMachine)
                       {
                           return InStateMachine->PointerUp(InputCoordinates);
                       }
                       return false;
                   });
}

FReply URiveWidget::NativeOnMouseMove(const FGeometry& InGeometry,
                                      const FPointerEvent& InMouseEvent)
{
    return OnInput(InGeometry,
                   InMouseEvent,
                   [this](const FVector2f& InputCoordinates,
                          FRiveStateMachine* InStateMachine) {
                       if (InStateMachine)
                       {
                           return InStateMachine->PointerMove(InputCoordinates);
                       }
                       return false;
                   });
}

void URiveWidget::SetAudioEngine(URiveAudioEngine* InRiveAudioEngine)
{
    if (RiveTextureObject)
    {
        RiveTextureObject->SetAudioEngine(InRiveAudioEngine);
        return;
    }

    UE_LOG(LogRive,
           Warning,
           TEXT("RiveObject was null while trying to SetAudioEngine"));
}

URiveArtboard* URiveWidget::GetArtboard() const
{
    if (RiveTextureObject && RiveTextureObject->GetArtboard())
    {
        return RiveTextureObject->GetArtboard();
    }

    return nullptr;
}

void URiveWidget::OnRiveObjectReady()
{
    if (RiveTextureObject)
    {
        RiveTextureObject->OnRiveReady.RemoveDynamic(
            this,
            &URiveWidget::OnRiveObjectReady);
    }

    if (!RiveWidget.IsValid() || !GetCachedWidget())
        return;

    if (auto artboard = RiveTextureObject->GetArtboard(); IsValid(artboard))
    {
        FVector2f ArtboardSize = artboard->GetSize();
        SetMinimumDesiredSize(FIntPoint(ArtboardSize.X, ArtboardSize.Y));
        RiveWidget->SetRiveTexture(RiveTextureObject);
        RiveDescriptor.ArtboardName = artboard->GetArtboardName();
        RiveDescriptor.StateMachineName = artboard->StateMachineName;
    }

    OnRiveReady.Broadcast();
}

FReply URiveWidget::OnInput(
    const FGeometry& MyGeometry,
    const FPointerEvent& MouseEvent,
    const TFunction<bool(const FVector2f&, FRiveStateMachine*)>&
        InStateMachineInputCallback)
{
    if (!RiveTextureObject || !RiveTextureObject->GetArtboard())
    {
        return FReply::Unhandled();
    }

    URiveArtboard* Artboard = RiveTextureObject->GetArtboard();
    if (!ensure(IsValid(Artboard)))
    {
        return FReply::Unhandled();
    }

    bool Result = false;

    Artboard->BeginInput();
    if (FRiveStateMachine* StateMachine = Artboard->GetStateMachine())
    {
        float ScaleFactor = -1.0f;

        if (RiveDescriptor.FitType == ERiveFitType::Layout)
            ScaleFactor = RiveDescriptor.ScaleFactor;

        FVector2f InputCoordinates =
            UE::Private::RiveWidget::GetInputCoordinates(RiveTextureObject,
                                                         Artboard,
                                                         MyGeometry,
                                                         MouseEvent,
                                                         ScaleFactor);
        Result = InStateMachineInputCallback(InputCoordinates, StateMachine);
    }
    Artboard->EndInput();

    return Result ? FReply::Handled() : FReply::Unhandled();
}

TArray<FString> URiveWidget::GetArtboardNamesForDropdown() const
{
    TArray<FString> Output;

    if (RiveDescriptor.RiveFile)
    {
        for (URiveArtboard* Artboard : RiveDescriptor.RiveFile->Artboards)
        {
            Output.Add(Artboard->GetArtboardName());
        }
    }

    return Output;
}

TArray<FString> URiveWidget::GetStateMachineNamesForDropdown() const
{
    TArray<FString> Output{""};
    if (RiveDescriptor.RiveFile)
    {
        for (URiveArtboard* Artboard : RiveDescriptor.RiveFile->Artboards)
        {
            if (Artboard->GetArtboardName().Equals(RiveDescriptor.ArtboardName))
            {
                Output.Append(Artboard->GetStateMachineNames());
                break;
            }
        }
    }

    return Output;
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
            GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardIndex) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardName))
    {
        TArray<FString> ArtboardNames = GetArtboardNamesForDropdown();
        if (ArtboardNames.Num() > 0 && RiveDescriptor.ArtboardIndex == 0 &&
            (RiveDescriptor.ArtboardName.IsEmpty() ||
             !ArtboardNames.Contains(RiveDescriptor.ArtboardName)))
        {
            RiveDescriptor.ArtboardName = ArtboardNames[0];
        }

        TArray<FString> StateMachineNames = GetStateMachineNamesForDropdown();
        if (StateMachineNames.Num() == 1)
        {
            RiveDescriptor.StateMachineName =
                StateMachineNames[0]; // No state machine, use blank
        }
        else if (RiveDescriptor.StateMachineName.IsEmpty() ||
                 !StateMachineNames.Contains(RiveDescriptor.StateMachineName))
        {
            RiveDescriptor.StateMachineName = StateMachineNames[1];
        }
    }
}
#endif

void URiveWidget::Setup()
{
    if (!RiveTextureObject || !RiveWidget.IsValid())
    {
        return;
    }

    RiveTextureObject->OnRiveReady.AddDynamic(this,
                                              &URiveWidget::OnRiveObjectReady);
#if WITH_EDITOR
    RiveTextureObject->bRenderInEditor = true;
#endif
    RiveTextureObject->Initialize(RiveDescriptor);
    CheckArtboardSize();
}

void URiveWidget::SetRiveDescriptor(const FRiveDescriptor& newDescriptor)
{
    if (RiveDescriptor.FitType == ERiveFitType::Layout &&
        newDescriptor.FitType != ERiveFitType::Layout)
        IsChangingFromLayout = true;

    RiveDescriptor = newDescriptor;

    Setup();
}

void URiveWidget::CheckArtboardSize()
{
    URiveArtboard* Artboard = GetArtboard();

    FVector2D WidgetSize = RiveWidget->GetSize();

    if (Artboard != nullptr && Artboard->IsInitialized())
    {
        if (RiveDescriptor.FitType == ERiveFitType::Layout)
        {
            FVector2f NewSize = FVector2f(
                static_cast<float>(WidgetSize.X) / RiveDescriptor.ScaleFactor,
                static_cast<float>(WidgetSize.Y) / RiveDescriptor.ScaleFactor);

            Artboard->SetSize(NewSize);
        }
        else
        {
            if (IsChangingFromLayout)
            {
                FVector2f NewSize = FVector2f(Artboard->GetOriginalSize().X,
                                              Artboard->GetOriginalSize().Y);

                Artboard->SetSize(NewSize);
            }

            IsChangingFromLayout = false;
        }
    }
}

void URiveWidget::OnSWidgetSizeChanged(const FVector2D& InNewSize)
{
    CheckArtboardSize();
}
#undef LOCTEXT_NAMESPACE
