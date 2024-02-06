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

	RootComponent->Mobility = EComponentMobility::Movable;
	
	ScreenUserWidget = CreateDefaultSubobject<URiveFullScreenUserWidget>(TEXT("ScreenUserWidget"));
	
	ScreenUserWidget->SetDisplayTypes(ERiveWidgetDisplayType::Viewport, ERiveWidgetDisplayType::Viewport, ERiveWidgetDisplayType::Viewport);
	
	ScreenUserWidget->SetRiveActor(this);

#if WITH_EDITOR
	
	RootComponent->bVisualizeComponent = true;

#endif // WITH_EDITOR

	RootComponent->SetMobility(EComponentMobility::Static);

	PrimaryActorTick.bCanEverTick = true;

	PrimaryActorTick.bStartWithTickEnabled = true;

	bAllowTickBeforeBeginPlay = true;

	SetActorTickEnabled(true);

	SetHidden(false);
}

void ARiveActor::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
	ScreenUserWidget->SetWidgetClass(InWidgetClass);
}

void ARiveActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

#if WITH_EDITOR
	
	bEditorDisplayRequested = true;

#endif //WITH_EDITOR
}

void ARiveActor::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR

	bEditorDisplayRequested = true;

#endif //WITH_EDITOR
}

void ARiveActor::PostActorCreated()
{
	Super::PostActorCreated();

#if WITH_EDITOR
	
	bEditorDisplayRequested = true;

#endif //WITH_EDITOR
}

void ARiveActor::Destroyed()
{
	if (ScreenUserWidget)
	{
		ScreenUserWidget->Hide();
	}

	Super::Destroyed();
}

void ARiveActor::BeginPlay()
{
	RequestGameDisplay();

	Super::BeginPlay();
}

void ARiveActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (ScreenUserWidget)
	{
		UWorld* ActorWorld = GetWorld();

		if (ActorWorld && (ActorWorld->WorldType == EWorldType::Game || ActorWorld->WorldType == EWorldType::PIE))
		{
			ScreenUserWidget->Hide();
		}
	}
}

void ARiveActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITOR

	if (bEditorDisplayRequested)
	{
		bEditorDisplayRequested = false;

		RequestEditorDisplay();
	}

#endif //WITH_EDITOR

	if (ScreenUserWidget)
	{
		ScreenUserWidget->Tick(DeltaSeconds);
	}
}

void ARiveActor::RequestEditorDisplay()
{
#if WITH_EDITOR

	UWorld* ActorWorld = GetWorld();

	if (ScreenUserWidget && ActorWorld && ActorWorld->WorldType == EWorldType::Editor)
	{
		ScreenUserWidget->Display(ActorWorld);
	}

#endif //WITH_EDITOR
}

void ARiveActor::RequestGameDisplay()
{
	UWorld* ActorWorld = GetWorld();
	
	if (ScreenUserWidget && ActorWorld && (ActorWorld->WorldType == EWorldType::Game || ActorWorld->WorldType == EWorldType::PIE))
	{
		ScreenUserWidget->Display(ActorWorld);
	}
}

UUserWidget* ARiveActor::GetUserWidget() const
{
	if (ScreenUserWidget)
	{
		return ScreenUserWidget->GetWidget();
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
