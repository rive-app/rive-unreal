// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "RiveActorComponent.generated.h"

class URiveFile;

UCLASS(ClassGroup = (Custom), Meta = (BlueprintSpawnableComponent))
class RIVE_API URiveActorComponent : public UActorComponent
{
    GENERATED_BODY()

    /**
     * Structor(s)
     */

public:

    // Sets default values for this component's properties
    URiveActorComponent();

    //~ BEGIN : UActorComponent Interface

protected:

    // Called when the game starts
    virtual void BeginPlay() override;

public:

    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //~ END : UActorComponent Interface

    /**
     * Attribute(s)
     */

public:

    /** Reference to Ava Blueprint Asset */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Rive)
    TObjectPtr<URiveFile> RiveFile;
};
