// Copyright Rive, Inc. All rights reserved.


#include "Rive/RiveTexture.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "RenderingThread.h"
#include "RiveArtboard.h"
#include "RiveTextureResource.h"

URiveTexture::URiveTexture()
{
    SRGB = true;

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
	if (!RiveRenderer)
	{
		UE_LOG(LogRive, Error, TEXT("RiveRenderer is null, unable to create the RiveTextureResource"));
		return nullptr;
	}
	
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
	if (InNewSize.X == SizeX && InNewSize.Y == SizeY)
	{
		return;
	}
	
	if (InNewSize.X <= RIVE_MIN_TEX_RESOLUTION || InNewSize.Y <= RIVE_MIN_TEX_RESOLUTION
		|| InNewSize.X >= RIVE_MAX_TEX_RESOLUTION || InNewSize.Y >= RIVE_MAX_TEX_RESOLUTION)
	{
		UE_LOG(LogRive, Warning, TEXT("Wrong Rive Texture Size X:%d, Y:%d"), InNewSize.X, InNewSize.Y);

		return;
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
	}
	else
	{
		const FMatrix Matrix = InArtboard->GetLastDrawTransformMatrix();
		const FVector LocalCoordinate = Matrix.InverseTransformPosition(FVector(TextureRelativePosition.X, TextureRelativePosition.Y, 0));
		return FVector2f(LocalCoordinate.X, LocalCoordinate.Y);
	}
}

ESimpleElementBlendMode URiveTexture::GetSimpleElementBlendMode() const
{
	ESimpleElementBlendMode NewBlendMode = ESimpleElementBlendMode::SE_BLEND_Opaque;

	switch (RiveBlendMode)
	{
	case ERiveBlendMode::SE_BLEND_Opaque:
		break;
	case ERiveBlendMode::SE_BLEND_Masked:
		NewBlendMode = SE_BLEND_Masked;
		break;
	case ERiveBlendMode::SE_BLEND_Translucent:
		NewBlendMode = SE_BLEND_Translucent;
		break;
	case ERiveBlendMode::SE_BLEND_Additive:
		NewBlendMode = SE_BLEND_Additive;
		break;
	case ERiveBlendMode::SE_BLEND_Modulate:
		NewBlendMode = SE_BLEND_Modulate;
		break;
	case ERiveBlendMode::SE_BLEND_MaskedDistanceField:
		NewBlendMode = SE_BLEND_MaskedDistanceField;
		break;
	case ERiveBlendMode::SE_BLEND_MaskedDistanceFieldShadowed:
		NewBlendMode = SE_BLEND_MaskedDistanceFieldShadowed;
		break;
	case ERiveBlendMode::SE_BLEND_TranslucentDistanceField:
		NewBlendMode = SE_BLEND_TranslucentDistanceField;
		break;
	case ERiveBlendMode::SE_BLEND_TranslucentDistanceFieldShadowed:
		NewBlendMode = SE_BLEND_TranslucentDistanceFieldShadowed;
		break;
	case ERiveBlendMode::SE_BLEND_AlphaComposite:
		NewBlendMode = SE_BLEND_AlphaComposite;
		break;
	case ERiveBlendMode::SE_BLEND_AlphaHoldout:
		NewBlendMode = SE_BLEND_AlphaHoldout;
		break;
	case ERiveBlendMode::SE_BLEND_AlphaBlend:
		NewBlendMode = SE_BLEND_AlphaBlend;
		break;
	}

	return NewBlendMode;
}


void URiveTexture::InitializeResources() const
{
	if (!UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
	{
		UE_LOG(LogRive, Error, TEXT("Failed to InitializeResources for the RiveTexture as we do not have a valid renderer."));
		return;
	}
	
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
