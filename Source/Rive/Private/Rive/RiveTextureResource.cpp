// Copyright Rive, Inc. All rights reserved.


#include "RiveTextureResource.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "Rive/RiveTexture.h"

namespace UE::Rive::Renderer
{
	class IRiveRenderer;
}

FRiveTextureResource::FRiveTextureResource(URiveTexture* Owner)
{
	RiveTexture = Owner;
}

void FRiveTextureResource::InitRHI(FRHICommandListBase& RHICmdList)
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (RiveTexture)
	{
		FSamplerStateInitializerRHI SamplerStateInitializer(
			(ESamplerFilter)UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings()->GetSamplerFilter(RiveTexture),
			AM_Border, AM_Border, AM_Wrap);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);

	}
}

void FRiveTextureResource::ReleaseRHI()
{
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	if (RiveTexture)
	{
		RHIUpdateTextureReference(RiveTexture->TextureReference.TextureReferenceRHI, nullptr);
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
