// Copyright Rive, Inc. All rights reserved.

#include "RiveActorFactory.h"
#include "Game/RiveActor.h"
#include "Rive/RiveFile.h"

URiveActorFactory::URiveActorFactory()
{
	DisplayName = NSLOCTEXT("RiveActorFactory", "RiveActorDisplayName", "RiveActor");

	NewActorClass = ARiveActor::StaticClass();
}

AActor* URiveActorFactory::SpawnActor(UObject* InAsset, ULevel* InLevel, const FTransform& InTransform, const FActorSpawnParameters& InSpawnParams)
{
	ARiveActor* NewActor = Cast<ARiveActor>(Super::SpawnActor(InAsset, InLevel, InTransform, InSpawnParams));

	if (NewActor)
	{
		if (URiveFile* RiveFile = Cast<URiveFile>(InAsset))
		{
			NewActor->RiveFile = RiveFile;

			NewActor->SetWidgetClass(RiveFile->GetWidgetClass());
		}
	}

	return NewActor;
}

bool URiveActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (AssetData.IsValid() && !AssetData.GetClass()->IsChildOf(URiveFile::StaticClass()))
	{
		OutErrorMsg = NSLOCTEXT("CanCreateActor", "NoRiveFileAsset", "A valid rive file asset must be specified.");
		
		return false;
	}

	return true;
}

UObject* URiveActorFactory::GetAssetFromActorInstance(AActor* ActorInstance)
{
	if (ARiveActor* RiveActor = Cast<ARiveActor>(ActorInstance))
	{
		return RiveActor->RiveFile;
	}

	return nullptr;
}
