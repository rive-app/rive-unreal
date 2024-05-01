// Copyright Rive, Inc. All rights reserved.

#include "Game/RiveActor.h"

#include "Rive/RiveFile.h"
#include "RiveWidget/RiveFullScreenUserWidget.h"

#define LOCTEXT_NAMESPACE "ARiveActor"

// Sets default values
ARiveActor::ARiveActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent0"));	
	check(RootComponent);
	
	AudioEngine = CreateDefaultSubobject<URiveAudioEngine>(TEXT("RiveAudioEngine"));
}

void ARiveActor::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
	UserWidgetClass = InWidgetClass;
}

void ARiveActor::PostLoad()
{
	Super::PostLoad();

	if (RiveFile)
	{
		RiveFile->SetAudioEngine(AudioEngine);
	}
}

void ARiveActor::BeginPlay()
{
	Super::BeginPlay();
	
	UWorld* ActorWorld = GetWorld();
	
	if (IsValid(RiveFile))
	{
		if (IsValid(ActorWorld) && (ActorWorld->WorldType == EWorldType::PIE))
		{
			RiveFile->InstantiateArtboard();
		}

		RiveFile->GetArtboard()->SetAudioEngine(AudioEngine);

		ScreenUserWidget = CreateWidget(ActorWorld, UserWidgetClass);
		ScreenUserWidget->AddToViewport();
	}
}

void ARiveActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::EndPlayInEditor && IsValid(RiveFile))
	{
		RiveFile->InstantiateArtboard();
	}
	
	Super::EndPlay(EndPlayReason);
}

#undef LOCTEXT_NAMESPACE
