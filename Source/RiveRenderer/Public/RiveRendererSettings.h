// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RiveRendererSettings.generated.h"

UCLASS(Config = Engine, DefaultConfig)
class RIVERENDERER_API URiveRendererSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    URiveRendererSettings();

    UPROPERTY(EditAnywhere,
              config,
              Category = "Rive Experimental Settings",
              DisplayName = "Enable RHI Technical Preview",
              META = (Tooltip = "Not available on Apple platforms or UE 5.3."))
    bool bEnableRHITechPreview;

    virtual FName GetCategoryName() const override
    {
        return FName(TEXT("Rive"));
    }

#if WITH_EDITOR
    virtual void PostInitProperties() override;
    virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

private:
    bool bCanUseRHI = true;
};
