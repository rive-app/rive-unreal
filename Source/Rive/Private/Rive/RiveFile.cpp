// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveFile.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "IRiveRenderTarget.h"
#include "RiveRendererUtils.h"
#include "Rive/RiveArtboard.h"

URiveFile::URiveFile()
{
    OverrideFormat = EPixelFormat::PF_R8G8B8A8;

    bCanCreateUAV = true;

    // TODO. We might need extra texture to Draw rive separately and copy to this

    SizeX = 500;
    SizeY = 500;
}

TStatId URiveFile::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URiveFile, STATGROUP_Tickables);
}

UE_DISABLE_OPTIMIZATION
void URiveFile::Tick(float InDeltaSeconds)
{
    if (bIsInitialized && bIsRendering)
    {
        // Maybe only once
        //if (bDrawOnceTest == false)
        {
#if WITH_RIVE

            if (rive::Artboard* NativeArtboard = RiveArtboard->GetNativeArtBoard())
            {
                RiveRenderTarget->DrawArtboard((uint8)RiveFitType, RiveAlignment.X, RiveAlignment.Y, NativeArtboard, DebugColor);

                bDrawOnceTest = true;

                // TODO. Move to state machine class
                RiveArtboard->AdvanceDefaultStateMachine(InDeltaSeconds);
            }

            // Copy from render target
            // TODO. move from here
            // Separate target might needed to let RIve draw only to separate teture
            {
                FTextureRenderTargetResource* RiveFileResource = GameThread_GetRenderTargetResource();
                FTextureRenderTargetResource* RiveFileRenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
                ENQUEUE_RENDER_COMMAND(CopyRenderTexture)(
        [this, RiveFileResource, RiveFileRenderTargetResource](FRHICommandListImmediate& RHICmdList)
                    {

                     UE::Rive::Renderer::FRiveRendererUtils::CopyTextureRDG(RHICmdList, RiveFileRenderTargetResource->TextureRHI, RiveFileResource->TextureRHI);
                    });
            }

#endif // WITH_RIVE
        }
    }
}
UE_ENABLE_OPTIMIZATION

bool URiveFile::IsTickable() const
{
    return FTickableGameObject::IsTickable();
}

uint32 URiveFile::CalcTextureMemorySizeEnum(ETextureMipCount Enum) const
{
    // Calculate size based on format.  All mips are resident on render targets so we always return the same value.
    EPixelFormat Format = GetFormat();
    int32 BlockSizeX = GPixelFormats[Format].BlockSizeX;
    int32 BlockSizeY = GPixelFormats[Format].BlockSizeY;
    int32 BlockBytes = GPixelFormats[Format].BlockBytes;
    int32 NumBlocksX = (SizeX + BlockSizeX - 1) / BlockSizeX;
    int32 NumBlocksY = (SizeY + BlockSizeY - 1) / BlockSizeY;
    int32 NumBytes = NumBlocksX * NumBlocksY * BlockBytes;
    return NumBytes;
}

void URiveFile::PostLoad()
{
    UObject::PostLoad();

    Initialize();
}

#if WITH_EDITOR
void URiveFile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
}
#endif // WITH_EDITOR

FLinearColor URiveFile::GetDebugColor() const
{
    return DebugColor;
}

void URiveFile::Initialize()
{
    RiveArtboard = NewObject<URiveArtboard>(this);

    RiveArtboard->LoadNativeArtboard(TempFileBuffer);

    if (UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
    {
        FVector2f ArtBoardSize = RiveArtboard->GetArtboardSize();
        
        InitCustomFormat( ArtBoardSize.X, ArtBoardSize.Y, OverrideFormat, false );

        RenderTarget = RiveRenderer->CreateDefaultRenderTarget(FIntPoint(ArtBoardSize.X, ArtBoardSize.Y));
        FlushRenderingCommands();
        RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(*GetPathName(), RenderTarget);

        RiveRenderTarget->Initialize();

        bIsInitialized = true;
    }
}

void URiveFile::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
    WidgetClass = InWidgetClass;
}
