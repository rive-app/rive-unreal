// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "Game/RiveWidgetActor.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Logs/RiveLog.h"
#include "UMG/RiveWidget.h"

#define LOCTEXT_NAMESPACE "ARiveActor"

// Sets default values
ARiveWidgetActor::ARiveWidgetActor()
{
    // Set this actor to call Tick() every frame.  You can turn this off to
    // improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    RootComponent =
        CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent0"));
    check(RootComponent);

    AudioEngine =
        CreateDefaultSubobject<URiveAudioEngine>(TEXT("RiveAudioEngine"));
}

void ARiveWidgetActor::BeginPlay()
{
    Super::BeginPlay();

    UWorld* ActorWorld = GetWorld();
    ScreenUserWidget = CreateWidget(ActorWorld, RiveWidgetClass);
    if (!ScreenUserWidget)
    {
        UE_LOG(LogRive, Error, TEXT("Failed to create ScreenUserWidget."));
        return;
    }

    ScreenUserWidget->AddToViewport();

    RiveWidget = Cast<URiveWidget>(ScreenUserWidget);
    if (IsValid(RiveWidget))
    {
        RiveWidget->SetAudioEngine(AudioEngine);
    }
    else if (UCanvasPanel* CanvasPanel =
                 Cast<UCanvasPanel>(ScreenUserWidget->WidgetTree->RootWidget))
    {
        RiveWidget = Cast<URiveWidget>(CanvasPanel->GetChildAt(0));
        if (RiveWidget)
        {
            RiveWidget->SetAudioEngine(AudioEngine);
        }
        else
        {
            UE_LOG(LogRive, Error, TEXT("Failed to find RiveWidget."));
        }
    }
    else
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Spawned Widget is not or does not contain a Rive Widget"));
    }
}

#undef LOCTEXT_NAMESPACE
