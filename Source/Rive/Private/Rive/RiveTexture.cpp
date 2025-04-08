// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveTexture.h"

#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "RenderingThread.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveTextureResource.h"

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
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("RiveRenderer is null, unable to create the "
                    "RiveTextureResource"));
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
    if (InNewSize.X < RIVE_MIN_TEX_RESOLUTION ||
        InNewSize.Y < RIVE_MIN_TEX_RESOLUTION ||
        InNewSize.X > RIVE_MAX_TEX_RESOLUTION ||
        InNewSize.Y > RIVE_MAX_TEX_RESOLUTION)
    {
        FIntPoint OldNewSize = InNewSize;
        InNewSize = {FMath::Clamp(InNewSize.X,
                                  RIVE_MIN_TEX_RESOLUTION,
                                  RIVE_MAX_TEX_RESOLUTION),
                     FMath::Clamp(InNewSize.Y,
                                  RIVE_MIN_TEX_RESOLUTION,
                                  RIVE_MAX_TEX_RESOLUTION)};
    }

    if (CurrentResource && InNewSize.X == Size.X && InNewSize.Y == Size.Y)
    {
        // Just making sure all internal data lines up
        SizeX = Size.X;
        SizeY = Size.Y;
        return;
    }

    // we need to make sure we are not drawing anything with the old size,
    // so we ensure all the commands are sent before going further
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

void URiveTexture::ResizeRenderTargets(const FVector2f InNewSize)
{
    ResizeRenderTargets(FIntPoint(InNewSize.X, InNewSize.Y));
}

FVector2f URiveTexture::GetLocalCoordinatesFromExtents(
    URiveArtboard* InArtboard,
    const FVector2f& InPosition,
    const FBox2f& InExtents) const
{
    const FVector2f RelativePosition = InPosition - InExtents.Min;
    const FVector2f Ratio{
        Size.X / InExtents.GetSize().X,
        SizeY / InExtents.GetSize().Y}; // Ratio should be the same for X and Y
    const FVector2f TextureRelativePosition = RelativePosition * Ratio;

    if (InArtboard->OnGetLocalCoordinate.IsBound())
    {
        return InArtboard->OnGetLocalCoordinate.Execute(
            InArtboard,
            TextureRelativePosition);
    }
    else
    {
        const FMatrix Matrix = InArtboard->GetLastDrawTransformMatrix();
        const FVector LocalCoordinate = Matrix.InverseTransformPosition(
            FVector(TextureRelativePosition.X, TextureRelativePosition.Y, 0));
        return FVector2f(LocalCoordinate.X, LocalCoordinate.Y);
    }
}

void URiveTexture::InitializeResources() const
{
    if (!IRiveRendererModule::Get().GetRenderer())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to InitializeResources for the RiveTexture as we "
                    "do not have a valid "
                    "renderer."));
        return;
    }

    ENQUEUE_RENDER_COMMAND(FRiveTextureResourceeUpdateTextureReference)
    ([this](FRHICommandListImmediate& RHICmdList) {
        IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
        FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

        FTextureRHIRef RenderableTexture;
        const FString DebugName = GetName();

        FRHITextureCreateDesc RenderTargetTextureDesc =
            FRHITextureCreateDesc::Create2D(*DebugName, Size.X, Size.Y, Format)
                .SetFlags(ETextureCreateFlags::UAV |
                          ETextureCreateFlags::Dynamic |
                          ETextureCreateFlags::ShaderResource |
                          ETextureCreateFlags::RenderTargetable);

#if !(PLATFORM_IOS || PLATFORM_MAC) // SRGB could have been manually overriden
        if (SRGB)
        {
            RenderTargetTextureDesc.AddFlags(ETextureCreateFlags::SRGB);
        }
#endif

        RenderableTexture = RHICreateTexture(RenderTargetTextureDesc);
        RenderableTexture->SetName(GetFName());
        CurrentResource->TextureRHI = RenderableTexture;

        RHIUpdateTextureReference(TextureReference.TextureReferenceRHI,
                                  CurrentResource->TextureRHI);
        // When the resource change, we need to tell the RiveFile otherwise
        // we will keep on drawing on an outdated RT
        OnResourceInitializedOnRenderThread.Broadcast(
            RHICmdList,
            CurrentResource->TextureRHI);
    });
}
