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

void URiveTexture::ResizeRenderTargets(FIntPoint InNewSize)
{
	if (InNewSize.X < RIVE_MIN_TEX_RESOLUTION || InNewSize.Y < RIVE_MIN_TEX_RESOLUTION
		|| InNewSize.X > RIVE_MAX_TEX_RESOLUTION || InNewSize.Y > RIVE_MAX_TEX_RESOLUTION)
	{
		FIntPoint OldNewSize = InNewSize;
		InNewSize = {
			FMath::Clamp(InNewSize.X, RIVE_MIN_TEX_RESOLUTION, RIVE_MAX_TEX_RESOLUTION),
			FMath::Clamp(InNewSize.Y, RIVE_MIN_TEX_RESOLUTION, RIVE_MAX_TEX_RESOLUTION)};
		UE_LOG(LogRive, Warning, TEXT("Wrong Rive Texture Size {X:%d, Y:%d} being changed to {X:%d, Y:%d}"), OldNewSize.X, OldNewSize.Y, InNewSize.X, InNewSize.Y);
	}
	
	if (InNewSize.X == Size.X && InNewSize.Y == Size.Y)
	{
		// Just making sure all internal data lines up
		SizeX = Size.X;
		SizeY = Size.Y;
		return;
	}

	// we need to make sure we are not drawing anything with the old size, so we ensure all the commands are sent before going further
	FlushRenderingCommands();
	
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

void URiveTexture::ResizeRenderTargets(const FVector2D InNewSize)
{
	ResizeRenderTargets(FIntPoint(InNewSize.X, InNewSize.Y));
}

FVector2D URiveTexture::GetLocalCoordinatesFromExtents(URiveArtboard* InArtboard, const FVector2D& InPosition, const FBox2D& InExtents) const
{
	const FVector2D RelativePosition = InPosition - InExtents.Min;
	const FVector2D Ratio { Size.X / InExtents.GetSize().X, SizeY / InExtents.GetSize().Y}; // Ratio should be the same for X and Y
	const FVector2D TextureRelativePosition = RelativePosition * Ratio;

	if (InArtboard->OnGetLocalCoordinate.IsBound())
	{
		return InArtboard->OnGetLocalCoordinate.Execute(InArtboard, TextureRelativePosition);
	}
	else
	{
		const FMatrix Matrix = InArtboard->GetLastDrawTransformMatrix();
		const FVector LocalCoordinate = Matrix.InverseTransformPosition(FVector(TextureRelativePosition.X, TextureRelativePosition.Y, 0));
		return FVector2D(LocalCoordinate.X, LocalCoordinate.Y);
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

		ETextureCreateFlags TextureFlags = TexCreate_Dynamic | TexCreate_ShaderResource | TexCreate_RenderTargetable;

#if !(PLATFORM_IOS || PLATFORM_MAC) //SRGB could hvae been manually overriden
		if (SRGB)
		{
			TextureFlags |= TexCreate_SRGB;
		}
#endif

		FRHIResourceCreateInfo RenderTargetTextureCreateInfo;
		RenderTargetTextureCreateInfo.ClearValueBinding = FClearValueBinding(FLinearColor(0.0f, 0.0f, 0.0f));
		
		RenderableTexture = RHICreateTexture2D(Size.X, Size.Y, Format, 1, 1, TextureFlags, RenderTargetTextureCreateInfo);
		RenderableTexture->SetName(GetFName());
		CurrentResource->TextureRHI = RenderableTexture;

		RHIUpdateTextureReference(TextureReference.TextureReferenceRHI, CurrentResource->TextureRHI);
		// When the resource change, we need to tell the RiveFile otherwise we will keep on drawing on an outdated RT
		OnResourceInitializedOnRenderThread.Broadcast(RHICmdList, CurrentResource->TextureRHI);
	});
}
