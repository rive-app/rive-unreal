// Copyright Rive, Inc. All rights reserved.


#include "Rive/RiveTexture.h"

#include "RiveTextureResource.h"
#include "Logs/RiveLog.h"

URiveTexture::URiveTexture()
{
#if PLATFORM_IOS
	SRGB = false;
#else
	SRGB = true;
#endif
	bIsResolveTarget = true;
	SamplerAddressMode = AM_Wrap;
#if PLATFORM_ANDROID
	Format = PF_R8G8B8A8_SNORM;
#else
	Format = PF_R8G8B8A8;
#endif
	Size.X = Size.Y = 500;
	SizeX = Size.X;
	SizeY = Size.Y;
}

FTextureResource* URiveTexture::CreateResource()
{
	//UTexture::ReleaseResource() calls the delete
	CurrentResource = new FRiveTextureResource(this);
	SetResource(CurrentResource);
	InitializeResources();

	return CurrentResource;
}

void URiveTexture::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
{
	Super::GetResourceSizeEx(CumulativeResourceSize);

	if (CurrentResource != nullptr)
	{
		CumulativeResourceSize.AddUnknownMemoryBytes(CurrentResource->GetResourceSize());
	}
}

void URiveTexture::PostLoad()
{
	Super::PostLoad();

	if (!IsRunningCommandlet())
	{
		if (!CurrentResource)
		{
			UpdateResource();
		}

		FlushRenderingCommands();
	}
}

void URiveTexture::BeginDestroy()
{
	Super::BeginDestroy();
}

void URiveTexture::ResizeRenderTargets(const FIntPoint InNewSize)
{
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

		if (SRGB)
		{
			RenderTargetTextureDesc.AddFlags(ETextureCreateFlags::SRGB);
		}

		if (bNoTiling)
		{
			RenderTargetTextureDesc.AddFlags(ETextureCreateFlags::NoTiling);
		}
	
		RenderableTexture = RHICreateTexture(RenderTargetTextureDesc);
		RenderableTexture->SetName(GetFName());
		CurrentResource->TextureRHI = RenderableTexture;

		RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, CurrentResource->TextureRHI);
	});
}
