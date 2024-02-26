﻿// Copyright Rive, Inc. All rights reserved.


#include "Rive/RiveTexture.h"

#include "RiveTextureResource.h"
#include "Logs/RiveLog.h"

URiveTexture::URiveTexture()
{
#if PLATFORM_IOS || PLATFORM_MAC
	SRGB = false;
#else
	SRGB = true;
#endif

#if PLATFORM_ANDROID
	Format = PF_R8G8B8A8_SNORM;
#else
	Format = PF_R8G8B8A8;
#endif
	
	SizeY = SizeX = Size.X = Size.Y = RIVE_STANDARD_TEX_RESOLUTION;
}

FTextureResource* URiveTexture::CreateResource()
{
	// UTexture::ReleaseResource() calls the delete
	CurrentResource = new FRiveTextureResource(this);
	SetResource(CurrentResource);
	InitializeResources();

	return CurrentResource;
}

void URiveTexture::PostLoad()
{
	Super::PostLoad();
	
	if (!IsRunningCommandlet())
	{
		ResizeRenderTargets(Size);
	}
}

void URiveTexture::ResizeRenderTargets(const FIntPoint InNewSize)
{
	if (!(ensure(InNewSize.X >= RIVE_MIN_TEX_RESOLUTION) || ensure(InNewSize.Y >= RIVE_MIN_TEX_RESOLUTION)
		|| ensure(InNewSize.X <= RIVE_MAX_TEX_RESOLUTION) || ensure(InNewSize.Y <= RIVE_MAX_TEX_RESOLUTION)))
	{
		UE_LOG(LogRive, Error, TEXT("Wrong Rive Texture Size X:%d, Y:%d"), InNewSize.X, InNewSize.Y);
	}

	
	SizeX = Size.X = InNewSize.X;
	SizeY = Size.Y = InNewSize.Y;
	
	if (!CurrentResource)
	{
		// Create Resource
		UpdateResource();
	}
	else
	{
		// Create new TextureRHI with new size
		InitializeResources();
	}

	FlushRenderingCommands();
}

void URiveTexture::ResizeRenderTargets(const FVector2f InNewSize)
{
	ResizeRenderTargets(FIntPoint(InNewSize.X, InNewSize.Y));
}

void URiveTexture::InitializeResources() const
{
	ENQUEUE_RENDER_COMMAND(FRiveTextureResourceeUpdateTextureReference)
	([this](FRHICommandListImmediate& RHICmdList) {
		FTextureRHIRef RenderableTexture;

		FRHITextureCreateDesc RenderTargetTextureDesc =
			FRHITextureCreateDesc::Create2D(*GetName(), Size.X, Size.Y, Format)
				.SetClearValue(FClearValueBinding(FLinearColor(0.0f, 0.0f, 0.0f)))
				.SetFlags(ETextureCreateFlags::Dynamic | ETextureCreateFlags::ShaderResource | ETextureCreateFlags::RenderTargetable);

#if !(PLATFORM_IOS || PLATFORM_MAC) //SRGB could hvae been manually overriden
		if (SRGB)
		{
			RenderTargetTextureDesc.AddFlags(ETextureCreateFlags::SRGB);
		}
#endif
		
		RenderableTexture = RHICreateTexture(RenderTargetTextureDesc);
		RenderableTexture->SetName(GetFName());
		CurrentResource->TextureRHI = RenderableTexture;

		RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, CurrentResource->TextureRHI);
		// When the resource change, we need to tell the RiveFile otherwise we will keep on drawing on an outdated RT
		OnResourceInitialized_RenderThread(RHICmdList, CurrentResource->TextureRHI);
	});
}