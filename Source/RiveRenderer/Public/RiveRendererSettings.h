// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RiveRendererSettings.generated.h"

/**
 *
 */
UCLASS(Config = Engine, DefaultConfig)
class RIVERENDERER_API URiveRendererSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    /**
     *
     */
    URiveRendererSettings();

    UPROPERTY(EditAnywhere,
              config,
              Category = "Rive Experimental Settings",
              DisplayName = "Enable RHI Technical Preview")
    bool bEnableRHITechPreview;

    virtual FName GetCategoryName() const override
    {
        return FName(TEXT("Rive"));
    }
};
