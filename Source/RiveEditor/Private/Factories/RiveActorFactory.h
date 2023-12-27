// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "RiveActorFactory.generated.h"

/**
 * 
 */
UCLASS()
class RIVEEDITOR_API URiveActorFactory : public UActorFactory
{
	GENERATED_BODY()

public:
	URiveActorFactory();
	
	//~ Begin UActorFactory Interface
	virtual AActor* SpawnActor(UObject* InAsset, ULevel* InLevel, const FTransform& InTransform, const FActorSpawnParameters& InSpawnParams) override;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
	//~ End UActorFactory Interface
};
