// Copyright Rive, Inc. All rights reserved.


#include "Rive/RiveTexture.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"

#include "RiveArtboard.h"
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
	UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
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
	if (Size == InNewSize)
	{
		return;
	}
	
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

FVector2f URiveTexture::GetLocalCoordinatesFromExtents(URiveArtboard* InArtboard, const FVector2f& InPosition, const FBox2f& InExtents) const
{
	const FVector2f RelativePosition = InPosition - InExtents.Min;
	const FVector2f Ratio { Size.X / InExtents.GetSize().X, SizeY / InExtents.GetSize().Y}; // Ratio should be the same for X and Y
	const FVector2f TextureRelativePosition = RelativePosition * Ratio;

	if (InArtboard->OnGetLocalCoordinate.IsBound())
	{
		return InArtboard->OnGetLocalCoordinate.Execute(InArtboard, TextureRelativePosition);
	} else
	{
		return FVector2f::ZeroVector;
	}
}

void URiveTexture::InitializeResources() const
{
	ENQUEUE_RENDER_COMMAND(FRiveTextureResourceeUpdateTextureReference)
	([this](FRHICommandListImmediate& RHICmdList) {
		UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();
		FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
		
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
		OnResourceInitializedOnRenderThread.Broadcast(RHICmdList, CurrentResource->TextureRHI);
	});
}
