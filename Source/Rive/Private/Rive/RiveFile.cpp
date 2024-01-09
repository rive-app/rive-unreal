// Copyright Rive, Inc. All rights reserved.

#include "Rive/RiveFile.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "IRiveRenderTarget.h"
#include "Logs/RiveLog.h"
#include "RiveRendererUtils.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/pls/pls_render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

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

            if (const UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
            {
                if (UE::Rive::Core::FURStateMachine* StateMachine = Artboard->GetStateMachine())
                {
                    if (!bIsReceivingInput)
                    {
                        StateMachine->Advance(InDeltaSeconds);
                    }
                }

                const FVector2f RiveAlignmentXY = GetRiveAlignment();
                RiveRenderTarget->AlignArtboard((uint8)RiveFitType, RiveAlignmentXY.X, RiveAlignmentXY.Y, Artboard->GetNativeArtboard(), DebugColor);

                RiveRenderTarget->DrawArtboard(Artboard->GetNativeArtboard(), DebugColor);

                bDrawOnceTest = true;
            }
            
            // Copy from render target
            // TODO. move from here
            // Separate target might be needed to let Rive draw only to separate texture
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
    // TODO. WE need custom implementation here to handle the Rive FileEditor Changes
}

#endif // WITH_EDITOR

FLinearColor URiveFile::GetDebugColor() const
{
    return DebugColor;
}

FVector2f URiveFile::GetLocalCoordinates(const FVector2f& InScreenPosition, const FBox2f& InScreenRect) const
{
#if WITH_RIVE

    if (const UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
    {
        const FVector2f RiveAlignmentXY = GetRiveAlignment();
        const rive::Mat2D Transform = rive::computeAlignment((rive::Fit)RiveFitType,
            rive::Alignment(RiveAlignmentXY.X, RiveAlignmentXY.Y),
            rive::AABB(InScreenRect.Min.X, InScreenRect.Min.Y, InScreenRect.Max.X, InScreenRect.Max.Y),
            Artboard->GetBounds());
    
        const rive::Vec2D ResultingVector = Transform.invertOrIdentity() * rive::Vec2D(InScreenPosition.X, InScreenPosition.Y);
    
        return { ResultingVector.x, ResultingVector.y };
    }

#endif // WITH_RIVE

    return FVector2f::ZeroVector;
}

FIntPoint URiveFile::CalculateRenderTextureSize(const FIntPoint& InViewportSize) const
{
    // TODO. Fully Implement
    FIntPoint ReturnSize = {SizeX, SizeY};
    switch (RiveFitType) {
    case ERiveFitType::Fill:
        ReturnSize = InViewportSize;
        break;
    case ERiveFitType::Contain:
        break;
    case ERiveFitType::Cover:
        break;
    case ERiveFitType::FitWidth:
        break;
    case ERiveFitType::FitHeight:
        break;
    case ERiveFitType::None:
        ReturnSize = {SizeX, SizeY};
        break;
    case ERiveFitType::ScaleDown:
        break;
    }
   
    return ReturnSize;
}

FIntPoint URiveFile::CalculateRenderTexturePosition(const FIntPoint& InViewportSize) const
{
    // TODO. Implement
    FIntPoint ReturnPosition = {0, 0};
    const FIntPoint TextureSize = {SizeX, SizeY};

    if (RiveFitType == ERiveFitType::Fill)
    {
        return ReturnPosition;
    }

    switch (RiveAlignment) {
    case ERiveAlignment::TopLeft:
        ReturnPosition = {0, 0};
        break;
    case ERiveAlignment::TopCenter:
        break;
    case ERiveAlignment::TopRight:
        break;
    case ERiveAlignment::CenterLeft:
        break;
    case ERiveAlignment::Center:
        {
            int32 XPos = 0;
            int32 YPos = 0;
            if (InViewportSize.X > TextureSize.X)
            {
                XPos = (InViewportSize.X - TextureSize.X) *.5f;
            }

            if (InViewportSize.Y > TextureSize.Y)
            {
                YPos = (InViewportSize.Y - TextureSize.Y) *.5f;
            }
            
            ReturnPosition = {XPos, YPos};
            break;
        }
    case ERiveAlignment::CenterRight:
        break;
    case ERiveAlignment::BottomLeft:
        break;
    case ERiveAlignment::BottomCenter:
        break;
    case ERiveAlignment::BottomRight:
        break;
    }

    return ReturnPosition;
}

FVector2f URiveFile::GetRiveAlignment() const
{
    switch(RiveAlignment) {
    case ERiveAlignment::TopLeft:
        return FRiveAlignment::TopLeft;
    case ERiveAlignment::TopCenter:
        return FRiveAlignment::TopCenter;
    case ERiveAlignment::TopRight:
        return FRiveAlignment::TopRight;
    case ERiveAlignment::CenterLeft:
        return FRiveAlignment::CenterLeft;
    case ERiveAlignment::Center:
        return FRiveAlignment::Center;
    case ERiveAlignment::CenterRight:
         return FRiveAlignment::CenterRight;
    case ERiveAlignment::BottomLeft:
        return FRiveAlignment::BottomLeft;
    case ERiveAlignment::BottomCenter:
        return FRiveAlignment::BottomCenter;
    case ERiveAlignment::BottomRight:
        return FRiveAlignment::BottomRight;
    }

    check(0);
    return FRiveAlignment::Center;
}

ESimpleElementBlendMode URiveFile::GetSimpleElementBlendMode() const
{
    switch (RiveBlendMode) {
    case ERiveBlendMode::SE_BLEND_Opaque:
        return SE_BLEND_Opaque;
    case ERiveBlendMode::SE_BLEND_Masked:
        return SE_BLEND_Masked;
    case ERiveBlendMode::SE_BLEND_Translucent:
        return SE_BLEND_Translucent;
    case ERiveBlendMode::SE_BLEND_Additive:
        return SE_BLEND_Additive;
    case ERiveBlendMode::SE_BLEND_Modulate:
        return SE_BLEND_Modulate;
    case ERiveBlendMode::SE_BLEND_MaskedDistanceField:
        return SE_BLEND_MaskedDistanceField;
    case ERiveBlendMode::SE_BLEND_MaskedDistanceFieldShadowed:
        return SE_BLEND_MaskedDistanceFieldShadowed;
    case ERiveBlendMode::SE_BLEND_TranslucentDistanceField:
        return SE_BLEND_TranslucentDistanceField;
    case ERiveBlendMode::SE_BLEND_TranslucentDistanceFieldShadowed:
        return SE_BLEND_TranslucentDistanceFieldShadowed;
    case ERiveBlendMode::SE_BLEND_AlphaComposite:
        return SE_BLEND_AlphaComposite;
    case ERiveBlendMode::SE_BLEND_AlphaHoldout:
        return SE_BLEND_AlphaHoldout;
    }

    check(0);
    return ESimpleElementBlendMode::SE_BLEND_Opaque; 
}

void URiveFile::Initialize()
{
    if (!LoadNativeFile())
    {
        UE_LOG(LogRive, Error, TEXT("Unable to load the native artboard. Check log for more details."));

        return;
    }

    if (UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer())
    {
        if (UE::Rive::Core::FURArtboard* Artboard = GetArtboard())
        {
            const FVector2f ArtboardSize = Artboard->GetSize();

            constexpr bool bInForceLinearGamma = false;

            InitCustomFormat(ArtboardSize.X, ArtboardSize.Y, OverrideFormat, bInForceLinearGamma);

            RenderTarget = RiveRenderer->CreateDefaultRenderTarget(FIntPoint(ArtboardSize.X, ArtboardSize.Y));

            FlushRenderingCommands();

            RiveRenderTarget = RiveRenderer->CreateTextureTarget_GameThread(*GetPathName(), RenderTarget);

            RiveRenderTarget->Initialize();

            bIsInitialized = true;
        }
    }
}

void URiveFile::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
    WidgetClass = InWidgetClass;
}

UE_DISABLE_OPTIMIZATION

UE::Rive::Core::FURArtboard* URiveFile::GetArtboard() const
{
    if (!UnrealRiveFile)
    {
        UE_LOG(LogRive, Error, TEXT("Could not retrieve native artboard as we have detected an empty rive file."));

        return nullptr;
    }

    return UnrealRiveFile->GetArtboard();
}

bool URiveFile::LoadNativeFile()
{
    if (!UE::Rive::Renderer::IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load native artboard as the required Rive Renderer Module is either missing or not loaded properly."));

        return false;
    }

    UE::Rive::Renderer::IRiveRenderer* RiveRenderer = UE::Rive::Renderer::IRiveRendererModule::Get().GetRenderer();

    if (!RiveRenderer)
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid renderer."));

        return false;
    }

    if (!RiveRenderer->IsInitialized())
    {
        UE_LOG(LogRive, Error, TEXT("Could not load native artboard as the required Rive Renderer is not initialized."));

        return false;
    }

    if (!UnrealRiveFile.IsValid())
    {
        UnrealRiveFile = MakeUnique<UE::Rive::Assets::FURFile>(TempFileBuffer.GetData(), TempFileBuffer.Num());
    }

#if WITH_RIVE

    if (rive::pls::PLSRenderContext* PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr())
    {
        rive::ImportResult ImportResult = UnrealRiveFile->Import(PLSRenderContext);

        if (ImportResult != rive::ImportResult::success)
        {
            UE_LOG(LogRive, Error, TEXT("Failed to import rive file."));

            return false;
        }
    }
    else
    {
        UE_LOG(LogRive, Error, TEXT("Failed to import rive file as we do not have a valid context."));

        return false;
    }

#endif // WITH_RIVE

    return true;
}

UE_ENABLE_OPTIMIZATION
