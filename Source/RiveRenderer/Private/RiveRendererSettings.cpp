// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RiveRendererSettings.h"
#include "Runtime/Launch/Resources/Version.h"

URiveRendererSettings::URiveRendererSettings() : bEnableRHITechPreview(false)
{
#if PLATFORM_APPLE || ENGINE_MAJOR_VERSION < 5 ||                              \
    (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4)
    bCanUseRHI = false;
#else
    bCanUseRHI = true;
#endif
}

#if WITH_EDITOR
void URiveRendererSettings::PostInitProperties()
{
    Super::PostInitProperties();

    if (!bCanUseRHI)
    {
        bEnableRHITechPreview = false;
        SaveConfig();
    }
}

bool URiveRendererSettings::CanEditChange(const FProperty* InProperty) const
{
    bool bParentValue = Super::CanEditChange(InProperty);

    if (InProperty && InProperty->GetFName() ==
                          GET_MEMBER_NAME_CHECKED(URiveRendererSettings,
                                                  bEnableRHITechPreview))
    {
        // Only allow editing if running on Windows AND UE 5.4+
        return bParentValue && bCanUseRHI;
    }

    return bParentValue;
}

#endif
