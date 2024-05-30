// Copyright Rive, Inc. All rights reserved.

#include "UMG/RiveWidget.h"
#include "Rive/RiveObject.h"
#include "Slate/SRiveWidget.h"

#define LOCTEXT_NAMESPACE "RiveWidget"

URiveWidget::~URiveWidget()
{
    if (RiveWidget != nullptr)
    {
        RiveWidget->SetRiveTexture(nullptr);
    }
    
    RiveWidget.Reset();

    if (RiveObject != nullptr)
    {
        RiveObject->MarkAsGarbage();
        RiveObject = nullptr;
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

    if (RiveObject != nullptr)
    {
        RiveObject->MarkAsGarbage();
        RiveObject = nullptr;
    }
}

TSharedRef<SWidget> URiveWidget::RebuildWidget()
{
    RiveWidget = SNew(SRiveWidget);
    Setup();
    return RiveWidget.ToSharedRef();
}

void URiveWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Setup();
}

void URiveWidget::SetAudioEngine(URiveAudioEngine* InAudioEngine)
{
    RiveObject->GetArtboard()->SetAudioEngine(InAudioEngine);
}

void URiveWidget::Setup()
{
    if (!RiveObject)
    {
        RiveObject = NewObject<URiveObject>();
        RiveObject->Initialize(RiveDescriptor);
    }
    
    RiveObject->RiveBlendMode = RiveBlendMode;
    RiveWidget->SetRiveTexture(RiveObject);
    RiveWidget->RegisterArtboardInputs({RiveObject->GetArtboard()});
    OnRiveReady.Broadcast();
}

#undef LOCTEXT_NAMESPACE
