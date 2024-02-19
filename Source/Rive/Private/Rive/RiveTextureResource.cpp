// Copyright Rive, Inc. All rights reserved.


#include "RiveTextureResource.h"

#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "Rive/RiveFile.h"

UE_DISABLE_OPTIMIZATION

FRiveTextureResource::FRiveTextureResource(URiveFile* Owner)
{
	RiveFile = Owner;
}

void FRiveTextureResource::InitRHI(FRHICommandListBase& RHICmdList)
{
	if (RiveFile)
	{
		FSamplerStateInitializerRHI SamplerStateInitializer(
			(ESamplerFilter)UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(RiveFile),
			AM_Border, AM_Border, AM_Wrap);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
	}
}

void FRiveTextureResource::ReleaseRHI()
{
	TextureRHI.SafeRelease();

	if (RiveFile)
	{
		RHIUpdateTextureReference(RiveFile->TextureReference.TextureReferenceRHI, nullptr);
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
	return CalcTextureSize(GetSizeX(), GetSizeY(), EPixelFormat::PF_A8R8G8B8, 1);
}

UE_ENABLE_OPTIMIZATION