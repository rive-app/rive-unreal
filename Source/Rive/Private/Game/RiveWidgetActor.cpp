// Copyright Rive, Inc. All rights reserved.

#include "Game/RiveWidgetActor.h"

#include "Rive/RiveTextureObject.h"
#include "UMG/RiveWidget.h"

#define LOCTEXT_NAMESPACE "ARiveActor"

// Sets default values
ARiveWidgetActor::ARiveWidgetActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent0"));	
	check(RootComponent);
	
	AudioEngine = CreateDefaultSubobject<URiveAudioEngine>(TEXT("RiveAudioEngine"));
}

void ARiveWidgetActor::BeginPlay()
{
	Super::BeginPlay();
	
	UWorld* ActorWorld = GetWorld();
	ScreenUserWidget = CreateWidget(ActorWorld, RiveWidgetClass);
	if (!ScreenUserWidget)
	{
		return;
	}

	
	if (URiveWidget* RiveWidget = Cast<URiveWidget>(ScreenUserWidget))
	{
		RiveWidget->SetAudioEngine(AudioEngine);
	}
	
	ScreenUserWidget->AddToViewport();
}

#undef LOCTEXT_NAMESPACE
