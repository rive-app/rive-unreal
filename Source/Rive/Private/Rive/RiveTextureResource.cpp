// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "Rive/RiveTextureResource.h"

#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "RiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "RenderUtils.h"
#include "Rive/RiveTexture.h"

FRiveTextureResource::FRiveTextureResource(URiveTexture* Owner) :
    RiveTexture(Owner)
{}

void FRiveTextureResource::InitRHI(FRHICommandListBase& RHICmdList)
{
    if (RiveTexture)
    {
        FSamplerStateInitializerRHI SamplerStateInitializer(
            (ESamplerFilter)UDeviceProfileManager::Get()
                .GetActiveProfile()
                ->GetTextureLODSettings()
                ->GetSamplerFilter(RiveTexture),
            AM_Border,
            AM_Border,
            AM_Wrap);
        SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
    }
}

void FRiveTextureResource::ReleaseRHI()
{
    if (RiveTexture)
    {
        RHIUpdateTextureReference(
            RiveTexture->TextureReference.TextureReferenceRHI,
            nullptr);
    }

    FTextureResource::ReleaseRHI();
}

uint32 FRiveTextureResource::GetSizeX() const
{
    return TextureRHI.IsValid() ? TextureRHI->GetSizeXYZ().X : 0;
}

uint32 FRiveTextureResource::GetSizeY() const
{
    return TextureRHI.IsValid() ? TextureRHI->GetSizeXYZ().Y : 0;
}

SIZE_T FRiveTextureResource::GetResourceSize()
{
    check(RiveTexture);
    return CalcTextureSize(GetSizeX(), GetSizeY(), RiveTexture->Format, 1);
}
