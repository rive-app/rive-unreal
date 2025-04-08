// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveTextureResource.h"

#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "RenderUtils.h"
#include "Rive/RiveTexture.h"

FRiveTextureResource::FRiveTextureResource(URiveTexture* Owner) :
    RiveTexture(Owner)
{}

void FRiveTextureResource::InitRHI(FRHICommandListBase& RHICmdList)
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!IRiveRendererModule::Get().GetRenderer())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to InitRHI for the RiveTextureResource as we do "
                    "not have a valid "
                    "renderer."));
        return;
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

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
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (ensure(RiveRenderer))
    {
        RiveRenderer->GetThreadDataCS().Lock();
    }

    FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

    if (RiveTexture)
    {
        RHIUpdateTextureReference(
            RiveTexture->TextureReference.TextureReferenceRHI,
            nullptr);
    }

    FTextureResource::ReleaseRHI();

    if (RiveRenderer)
    {
        RiveRenderer->GetThreadDataCS().Unlock();
    }
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
