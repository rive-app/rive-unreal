// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveFile.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "IRiveRenderTarget.h"
#include "Rive/RiveArtboard.h"

URiveFile::URiveFile()
{
    OverrideFormat = EPixelFormat::PF_R8G8B8A8;

    bCanCreateUAV = true;

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
    if (IsRendering())
    {
        // Maybe only once
        //if (bDrawOnceTest == false)
        {
#if WITH_RIVE

            if (rive::Artboard* NativeArtboard = RiveArtboard->GetNativeArtBoard())
            {
                RiveRenderTarget->DrawArtboard((uint8)RiveFitType, RiveAlignment.X, RiveAlignment.Y, NativeArtboard, DebugColor);
                

                bDrawOnceTest = true;

               //if (!isStatic)
                {
                    RiveArtboard->AdvanceDefaultStateMachine(InDeltaSeconds);
                    //double TimeNow = FPlatformTime::Seconds();
                    //double TimeToSet = TimeNow - LastTime;
                    // m_stateMachine?.advance((float)(now - m_lastTime));
                    // m_lastTime = now;
                }
            }

#endif // WITH_RIVE
        }

        CountdownRenderingTickCounter--;
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
        FlushRenderingCommands();
        RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(*GetPathName(), this);

        RiveRenderTarget->Initialize();

        bIsInitialized = true;
    }
}

bool URiveFile::IsRendering() const
{
    return CountdownRenderingTickCounter > 0;
}

void URiveFile::RequestRendering()
{
    CountdownRenderingTickCounter = CountdownRenderingTicks;
}

void URiveFile::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
    WidgetClass = InWidgetClass;
}
