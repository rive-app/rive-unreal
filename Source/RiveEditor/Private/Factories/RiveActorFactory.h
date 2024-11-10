// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "ActorFactories/ActorFactory.h"
#include "RiveActorFactory.generated.h"

/**
 *
 */
UCLASS()
class RIVEEDITOR_API URiveActorFactory : public UActorFactory
{
    GENERATED_BODY()

    /**
     * Structor(s)
     */

public:
    URiveActorFactory();

    //~ BEGIN : UActorFactory Interface

public:
    virtual AActor* SpawnActor(
        UObject* InAsset,
        ULevel* InLevel,
        const FTransform& InTransform,
        const FActorSpawnParameters& InSpawnParams) override;

    virtual bool CanCreateActorFrom(const FAssetData& AssetData,
                                    FText& OutErrorMsg) override;

    virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;

    //~ END : UActorFactory Interface
};
